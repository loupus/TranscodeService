#include <iostream>

class TranscoderCBArgument
{
    public:
    bool Success=true;
    std::string ErrMessage="";
    std::string AssetId="";
    std::string ProxyFile = "";
};

class ITranscoderCallBack
{
    public:
    virtual void operator()(TranscoderCBArgument pArg)=0;   
    virtual void Call(TranscoderCBArgument pArg)=0;
    virtual ~ITranscoderCallBack() = default;
};


template <typename T>
class CTransCoderCallBack : public ITranscoderCallBack
{
    public:
    typedef void (T::*Method)(TranscoderCBArgument);
    CTransCoderCallBack(T* pObj, Method pMethod)
   : obj_(pObj),
     method_(pMethod)
   {}

   ~CTransCoderCallBack() {}
    void operator() (TranscoderCBArgument pArg) override
   {
      return (obj_->*method_)(pArg);
   }

   void Call (TranscoderCBArgument pArg) override
   {
      return (obj_->*method_)(pArg);
   }

private:

   T* obj_;
   Method method_;
};

