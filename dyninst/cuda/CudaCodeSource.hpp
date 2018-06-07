#include <CodeSource.h>

namespace Dyninst {
namespace ParseAPI {

class PARSER_EXPORT CudaCodeSource : public CodeSource {
 public:
    CudaCodeSource() {};
    virtual ~CudaCodeSource();

 public:
    /** CodeSource Implementation **/
    virtual bool nonReturning(Address /*func_entry*/);
    virtual bool nonReturningSyscall(int /*number*/);
    virtual Address baseAddress();
    virtual Address loadAddress();
    virtual Address getTOC(Address);

    virtual void print_stats() const;
    virtual bool have_stats() const;

    virtual void incrementCounter(const std::string& /*name*/) const;
    virtual void addCounter(const std::string& /*name*/, int /*num*/) const;
    virtual void decrementCounter(const std::string& /*name*/) const;
    virtual void startTimer(const std::string& /*name*/) const;
    virtual void stopTimer(const std::string& /*name*/) const;
    virtual bool findCatchBlockByTryRange(Address /*given try address*/, std::set<Address> & /* catch start */)  const;

    /** InstructionSource implementation **/
    bool isValidAddress(const Address) const;
    void* getPtrToInstruction(const Address) const;
    void* getPtrToData(const Address) const;
    unsigned int getAddressWidth() const;
    bool isCode(const Address) const;
    bool isData(const Address) const;
    bool isReadOnly(const Address) const;
    Address offset() const;
    Address length() const;
    Architecture getArch() const;
};


class PARSER_EXPORT CudaCodeRegion : public CodeRegion {
 public:
    CudaCodeRegion();
    ~CudaCodeRegion();

    /** CodeRegion implementation **/
    void names(Address, std::vector<std::string> &);
    bool findCatchBlock(Address addr, Address & catchStart);

    Address low() const;
    Address high() const;

    /** InstructionSource implementation **/
    bool isValidAddress(const Address) const;
    void* getPtrToInstruction(const Address) const;
    void* getPtrToData(const Address) const;
    unsigned int getAddressWidth() const;
    bool isCode(const Address) const;
    bool isData(const Address) const;
    bool isReadOnly(const Address) const;
    Address offset() const;
    Address length() const;
    Architecture getArch() const;
};

}
}
