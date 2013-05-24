
#include "executor.hpp"

#include <com/sun/star/system/SystemShellExecuteFlags.hpp>

namespace mytools
{

ExecutionThread::ExecutionThread( 
        const uno::Reference< system::XSystemShellExecute > & xSse, 
        const OUString & sCommand, 
        const OUString & sArguments )
: m_xSystemShellExecute( xSse ), 
  m_sCommand( sCommand ), 
  m_sArguments( sArguments )
{
}

ExecutionThread::~ExecutionThread()
{
}

void SAL_CALL ExecutionThread::run()
{
    try
    {
        m_xSystemShellExecute->execute( 
            m_sCommand, m_sArguments, 
            system::SystemShellExecuteFlags::NO_SYSTEM_ERROR_MESSAGE ); 
    }
    catch (...)
    {
        onTerminated();
    }
}

void SAL_CALL ExecutionThread::onTerminated()
{
    delete this;
}

} // namespace
