#ifndef __NATIVE_ASYNC_ACTIONCALLBACK_H__
#define __NATIVE_ASYNC_ACTIONCALLBACK_H__

/*-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
 * Propose :
 * -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.*/

#include "../helper/TemplateSeqInd.h"
#include "FutureError.h"

namespace native {

template<class R>
class FutureShared;

/** Action callback base class template
 */
template<typename P>
class ActionCallbackBase {
public:
    virtual ~ActionCallbackBase() {}

    static void SetValue(std::shared_ptr<ActionCallbackBase<P>> iInstance, P&& p);
    static void SetError(std::shared_ptr<ActionCallbackBase<P>> iInstance, const FutureError&);

    virtual void setValueCb(P&&) = 0;
    virtual void setErrorCb(const FutureError&) = 0;
    virtual std::shared_ptr<uv_loop_t> getLoop() = 0;
};

/** Acction callback base class template specialization for void type.
 */
template<>
class ActionCallbackBase<void> {
protected:

public:
    virtual ~ActionCallbackBase() {}

    // TODO: resolve why method from specialization class cannot be decouple
    template<typename T>
    static void SetValueT(std::shared_ptr<ActionCallbackBase<void>> iInstance);

    template<typename T>
    static void SetErrorT(std::shared_ptr<ActionCallbackBase<void>> iInstance, const FutureError &iError);

    static void SetValue(std::shared_ptr<ActionCallbackBase<void>> iInstance) {
        SetValueT<void>(iInstance);
    }

    static void SetError(std::shared_ptr<ActionCallbackBase<void>> iInstance, const FutureError &iError) {
        SetErrorT<void>(iInstance, iError);
    }

    virtual void setValueCb() = 0;
    virtual void setErrorCb(const FutureError&) = 0;
    virtual std::shared_ptr<uv_loop_t> getLoop() = 0;
};

template<typename R, typename... Args>
class ActionCallback: public ActionCallbackBase<void> {
    std::function<R(Args...)> _f;
    std::tuple<Args...> _args;
    std::shared_ptr<FutureShared<R>> _future;

    template<std::size_t... Is>
    void callFn(helper::TemplateSeqInd<Is...>);

    void setValueCb() override;
    void setErrorCb(const FutureError& iError) override;

public:
    ActionCallback() = delete;
    ActionCallback(std::shared_ptr<uv_loop_t> iLoop, std::function<R(Args...)> f, Args&&... args);

    std::shared_ptr<FutureShared<R>> getFuture();
    std::shared_ptr<uv_loop_t> getLoop() override;
};

template<typename... Args>
class ActionCallback<void, Args...>: public ActionCallbackBase<void> {
    std::function<void(Args...)> _f;
    std::tuple<Args...> _args;
    std::shared_ptr<FutureShared<void>> _future;

    template<std::size_t... Is>
    void callFn(helper::TemplateSeqInd<Is...>);

    void setValueCb() override;
    void setErrorCb(const FutureError& iError) override;

public:
    ActionCallback() = delete;
    ActionCallback(std::shared_ptr<uv_loop_t> iLoop, std::function<void(Args...)> f, Args&&... args);

    std::shared_ptr<FutureShared<void>> getFuture();
    std::shared_ptr<uv_loop_t> getLoop() override;
};

/** Value resolver callback class template. It call the function callback in case if the future resolved
 */
template<typename R, typename P, typename... Args>
class ActionCallbackP1: public ActionCallbackBase<P> {
    std::function<R(P, Args...)> _f;
    std::tuple<Args...> _args;
    std::shared_ptr<FutureShared<R>> _future;

    template<std::size_t... Is>
    void callFn(P&& p, helper::TemplateSeqInd<Is...>);

    void setValueCb(P&& p) override;
    void setErrorCb(const FutureError& iError) override;

public:
    ActionCallbackP1() = delete;
    ActionCallbackP1(std::shared_ptr<uv_loop_t> iLoop, std::function<R(P, Args...)> f, Args&&... args);

    std::shared_ptr<FutureShared<R>> getFuture();
    std::shared_ptr<uv_loop_t> getLoop() override;
};

template<typename P, typename... Args>
class ActionCallbackP1<void, P, Args...>: public ActionCallbackBase<P> {
    std::function<void(P, Args...)> _f;
    std::tuple<Args...> _args;
    std::shared_ptr<FutureShared<void>> _future;

    template<std::size_t... Is>
    void callFn(P&& p, helper::TemplateSeqInd<Is...>);

    void setValueCb(P&& p) override;
    void setErrorCb(const FutureError& iError) override;

public:
    ActionCallbackP1() = delete;
    ActionCallbackP1(std::shared_ptr<uv_loop_t> iLoop, std::function<void(P, Args...)> f, Args&&... args);

    std::shared_ptr<FutureShared<void>> getFuture();
    std::shared_ptr<uv_loop_t> getLoop() override;
};

// Errors

template<typename... Args>
class ActionCallbackError: public ActionCallbackBase<void> {
    std::function<void(const FutureError&, Args...)> _f;
    std::tuple<Args...> _args;
    std::shared_ptr<FutureShared<void>> _future;

    template<std::size_t... Is>
    void callFn(const FutureError& iError, helper::TemplateSeqInd<Is...>);

    void setValueCb() override;
    void setErrorCb(const FutureError& iError) override;

public:
    ActionCallbackError() = delete;
    ActionCallbackError(std::shared_ptr<uv_loop_t> iLoop, std::function<void(const FutureError&, Args...)> f, Args&&... args);

    std::shared_ptr<FutureShared<void>> getFuture();
    std::shared_ptr<uv_loop_t> getLoop() override;
};

/** Value resolver callback class template. It call the function callback in case if the future resolved
 */
template<typename R, typename... Args>
class ActionCallbackErrorP1: public ActionCallbackBase<R> {
    std::function<R(const FutureError&, Args...)> _f;
    std::tuple<Args...> _args;
    std::shared_ptr<FutureShared<R>> _future;

    template<std::size_t... Is>
    void callFn(const FutureError& iError, helper::TemplateSeqInd<Is...>);

    void setValueCb(R&& r) override;
    void setErrorCb(const FutureError& iError) override;

public:
    ActionCallbackErrorP1() = delete;
    ActionCallbackErrorP1(std::shared_ptr<uv_loop_t> iLoop, std::function<R(const FutureError&, Args...)> f, Args&&... args);

    std::shared_ptr<FutureShared<R>> getFuture();
    std::shared_ptr<uv_loop_t> getLoop() override;
};

} /* namespace native */

#endif /* __NATIVE_ASYNC_ACTIONCALLBACK_H__ */
