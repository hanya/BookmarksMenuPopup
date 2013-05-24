
#ifndef _THREAD_HPP_
#define _THREAD_HPP_

#include <osl/thread.hxx>

#include <com/sun/star/system/XSystemShellExecute.hpp>

using ::rtl::OUString;
using namespace ::com::sun::star;

namespace mytools
{

class ExecutionThread : public osl::Thread
{
private:

    const uno::Reference< system::XSystemShellExecute > m_xSystemShellExecute;
    const OUString m_sCommand;
    const OUString m_sArguments;
    
public:
    
    ExecutionThread( const uno::Reference< system::XSystemShellExecute > & xSse, 
            const OUString & sCommand, const OUString & sArguments );
    
    virtual ~ExecutionThread();
    
    virtual void SAL_CALL run();
    
    virtual void SAL_CALL onTerminated();
};

} // namespace

#endif
