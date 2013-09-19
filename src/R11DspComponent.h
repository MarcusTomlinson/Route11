#ifndef R11DSPCOMPONENT_H
#define R11DSPCOMPONENT_H

//-----------------------------------------------------------------------------

#include "DSPatch.h"
#include "R11AsyncProcessThread.h"

#include <cstdint>
#include <functional>

//=============================================================================

namespace Route11
{

template< typename PT >
class R11DspComponent final : public DspComponent
{
private:
  PT _process;

public:
  explicit R11DspComponent( int_fast8_t threadCount = 0, bool startRunning = false );
  explicit R11DspComponent( bool startRunning );
  R11DspComponent( R11ThreadConfig threadConfig, bool startRunning = false );

  void SetThreadCount( int_fast8_t threadCount );

  template< uint_fast16_t input, typename T >
  void SetInput( const T& value );

  template< uint_fast16_t input >
  auto GetInput() -> decltype( _process.template GetInput< input >() );

  template< uint_fast16_t output >
  auto GetOutput() -> decltype( _process.template GetOutput< output >() );

protected:
  virtual void Process_( DspSignalBus& inputs, DspSignalBus& outputs ) override;

private:
  // Fancy little trick to get around function template specialization within a class
  template< uint_fast16_t First, uint_fast16_t Size >
  struct _StaticLoop
  {
    void AddIos( std::function< void() > addMethod );
    void FillInputs( PT& component, DspSignalBus& inputs );
    void FillOutputs( PT& component, DspSignalBus& outputs );
  };

  template< uint_fast16_t N >
  struct _StaticLoop< N, N >
  {
    void AddIos( std::function< void() > addMethod ) { unused( addMethod ); }
    void FillInputs( PT& component, DspSignalBus& inputs ) { unused( component, inputs ); }
    void FillOutputs( PT& component, DspSignalBus& inputs ) { unused( component, inputs ); }
  };

private:
  _StaticLoop< 0, PT::inputCount > _inputsLooper;
  _StaticLoop< 0, PT::outputCount > _outputsLooper;
};

//=============================================================================

template< typename PT >
R11DspComponent< PT >::R11DspComponent( int_fast8_t threadCount, bool startRunning )
{
  _inputsLooper.AddIos( [ this ]() { AddInput_(); } );
  _outputsLooper.AddIos( [ this ]() { AddOutput_(); } );

  SetThreadCount( threadCount );

  if( startRunning )
  {
    StartAutoTick();
  }
}

//-----------------------------------------------------------------------------

template< typename PT >
R11DspComponent< PT >::R11DspComponent( bool startRunning )
: R11DspComponent( 0, startRunning ){}

//-----------------------------------------------------------------------------

template< typename PT >
R11DspComponent< PT >::R11DspComponent( R11ThreadConfig threadConfig, bool startRunning )
: R11DspComponent( threadConfig == R11ThreadConfig::ThreadPerCore ?
                   std::thread::hardware_concurrency() : 0,
                   startRunning ) {}

//=============================================================================

template< typename PT >
void R11DspComponent< PT >::SetThreadCount( int_fast8_t threadCount )
{
  if( threadCount < 0 )
  {
    return;
  }

  PauseAutoTick();
  _process.SetThreadCount( threadCount );
  ResumeAutoTick();
}

//-----------------------------------------------------------------------------

template< typename PT >
template< uint_fast16_t input, typename T >
void R11DspComponent< PT >::SetInput( const T& value )
{
  PauseAutoTick();
  _process.template SetInput< input >( value );
  ResumeAutoTick();
}

//-----------------------------------------------------------------------------

template< typename PT >
template< uint_fast16_t input >
auto R11DspComponent< PT >::GetInput() -> decltype( _process.template GetInput< input >() )
{
  PauseAutoTick();
  auto returnValue = _process.template GetInput< input >();
  ResumeAutoTick();

  return returnValue;
}

//-----------------------------------------------------------------------------

template< typename PT >
template< uint_fast16_t output >
auto R11DspComponent< PT >::GetOutput() -> decltype( _process.template GetOutput< output >() )
{
  PauseAutoTick();
  auto returnValue = _process.template GetOutput< output >();
  ResumeAutoTick();

  return returnValue;
}

//=============================================================================

template< typename PT >
void R11DspComponent< PT >::Process_( DspSignalBus& inputs, DspSignalBus& outputs )
{
  _inputsLooper.FillInputs( _process, inputs );

  _process.Tick();

  _outputsLooper.FillOutputs( _process, outputs );
}

//=============================================================================

template< typename PT >
template< uint_fast16_t First, uint_fast16_t Size >
void R11DspComponent< PT >::_StaticLoop< First, Size >::AddIos( std::function< void() > addMethod )
{
  addMethod();

  _StaticLoop< First + 1, Size >().AddIos( addMethod );
}

//-----------------------------------------------------------------------------

template< typename PT >
template< uint_fast16_t First, uint_fast16_t Size >
void R11DspComponent< PT >::_StaticLoop< First, Size >::FillInputs( PT& component, DspSignalBus& inputs )
{
  auto input = component.template GetInput< First >();

  inputs.GetValue( First, input );
  component.template SetInput< First >( input );

  _StaticLoop< First + 1, Size >().FillInputs( component, inputs );
}

//-----------------------------------------------------------------------------

template< typename PT >
template< uint_fast16_t First, uint_fast16_t Size >
void R11DspComponent< PT >::_StaticLoop< First, Size >::FillOutputs( PT& component, DspSignalBus& outputs )
{
  outputs.SetValue( First, component.template GetOutput< First >() );

  _StaticLoop< First + 1, Size >().FillOutputs( component, outputs );
}

}

//=============================================================================

#endif // R11DSPCOMPONENT_H