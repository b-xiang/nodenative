#ifndef __NATIVE_ASYNC_FEATURESHARED_H__
#define __NATIVE_ASYNC_FEATURESHARED_H__

#include <vector>
#include <memory>

namespace native {

template<class R>
class FutureShared {
private:
    std::vector<std::shared_ptr<ActionCallbackBase<R>>> _actions;
    bool _satisfied;

public:
    typedef R result_type;

    FutureShared() : _satisfied(false) {}

    void setValue(R&& iVal);
    void setError(const FutureError& iError);

    template<class F, typename... Args>
    std::shared_ptr<FutureShared<typename std::result_of<F(R, Args...)>::type>>
    then(F&& f, Args&&... args);

    template<class F, typename... Args>
    std::shared_ptr<FutureShared<R>>
    error(F&& f, Args&&... args);
};

template<>
class FutureShared<void> {
private:
    std::vector<std::shared_ptr<ActionCallbackBase<void>>> _actions;
    bool _satisfied;

    // TODO: without template method it doesn't allow to decouple the implementation method
    template<typename T>
    void setValueT();

    template<typename T>
    void setErrorT(const FutureError &iError);
public:
    typedef void result_type;

    FutureShared() : _satisfied(false) {}

    // TODO: without template method it doesn't allow to decouple the implementation method.
    void setValue() {
        setValueT<void>();
    }

    void setError(const FutureError& iError) {
        setErrorT<void>(iError);
    }

    template<class F, typename... Args>
    std::shared_ptr<FutureShared<typename std::result_of<F(Args...)>::type>>
    then(F&& f, Args&&... args);

    template<class F, typename... Args>
    std::shared_ptr<FutureShared<void>>
    error(F&& f, Args&&... args);
};

} /* namespace native */

#endif // __NATIVE_ASYNC_FEATURESHARED_H__
