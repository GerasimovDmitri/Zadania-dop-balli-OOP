#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <list>
#include <array>
#include <typeinfo>

class PipelineStepBase {
public:
    virtual void execute() = 0;
    virtual ~PipelineStepBase() = default;
};

template<typename T>
class InitialStep : public PipelineStepBase {
private:
    T value;
    bool executed = false;
public:
    InitialStep(T val) : value(val) {}
    void execute() override {
        executed = true;
    }
    T getValue() const { return value; }
};

class IResultHolder {
public:
    virtual ~IResultHolder() = default;
};

template<typename T>
class ResultHolder : public IResultHolder {
private:
    T result;
    bool has_result = false;
    
public:
    void setResult(T res) {
        result = res;
        has_result = true;
    }
    
    T getResult() const {
        if (!has_result) {
            throw std::runtime_error("Result not set");
        }
        return result;
    }
};

template<typename In, typename Out>
class TransformStep : public PipelineStepBase, public ResultHolder<Out> {
private:
    std::unique_ptr<PipelineStepBase> previous;
    std::function<Out(In)> func;
    bool executed = false;
    
public:
    TransformStep(std::unique_ptr<PipelineStepBase> prev, std::function<Out(In)> f) 
        : previous(std::move(prev)), func(f) {}
    
    void execute() override {
        if (!executed) {
            previous->execute();
          
            In input_value;
            
            if (auto* prevStep = dynamic_cast<InitialStep<In>*>(previous.get())) {
                input_value = prevStep->getValue();
            } 
            else if (auto* resultHolder = dynamic_cast<ResultHolder<In>*>(previous.get())) {
                input_value = resultHolder->getResult();
            }
            else {
                throw std::runtime_error("Cannot get value from previous step");
            }
            
            this->setResult(func(input_value));
            executed = true;
        }
    }
};

template<typename In>
class TerminalStep : public PipelineStepBase {
private:
    std::unique_ptr<PipelineStepBase> previous;
    std::function<void(In)> func;
    bool executed = false;
    
public:
    TerminalStep(std::unique_ptr<PipelineStepBase> prev, std::function<void(In)> f) 
        : previous(std::move(prev)), func(f) {}
    
    void execute() override {
        if (!executed) {
            previous->execute();
          
            In input_value;
            
            if (auto* prevStep = dynamic_cast<InitialStep<In>*>(previous.get())) {
                input_value = prevStep->getValue();
            } 
            else if (auto* resultHolder = dynamic_cast<ResultHolder<In>*>(previous.get())) {
                input_value = resultHolder->getResult();
            }
            else {
                throw std::runtime_error("Cannot get value from previous step");
            }
            
            func(input_value);
            executed = true;
        }
    }
};

template<typename T>
class Pipeline {
private:
    std::unique_ptr<PipelineStepBase> step;
    bool immediate_execution;
    
public:
    Pipeline(std::unique_ptr<PipelineStepBase> s, bool immediate = false) 
        : step(std::move(s)), immediate_execution(immediate) {
        if (immediate_execution) {
            execute();
        }
    }
    
    Pipeline(Pipeline&& other) noexcept = default;
    Pipeline& operator=(Pipeline&& other) noexcept = default;
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    
    void execute() {
        step->execute();
    }
    
    void operator()() {
        execute();
    }
    
    template<typename F>
    auto operator|(F&& func) {
        using InputType = T;
        using FunctionResult = decltype(func(std::declval<InputType>()));
        
        if constexpr (std::is_void_v<FunctionResult>) {
            auto terminalStep = std::make_unique<TerminalStep<InputType>>(
                std::move(step), std::forward<F>(func));
            return Pipeline<void>(std::move(terminalStep), immediate_execution);
        } else {
            auto transformStep = std::make_unique<TransformStep<InputType, FunctionResult>>(
                std::move(step), std::forward<F>(func));
            return Pipeline<FunctionResult>(std::move(transformStep), immediate_execution);
        }
    }
};

template<>
class Pipeline<void> {
private:
    std::unique_ptr<PipelineStepBase> step;
    bool immediate_execution;
    
public:
    Pipeline(std::unique_ptr<PipelineStepBase> s, bool immediate = false) 
        : step(std::move(s)), immediate_execution(immediate) {
        if (immediate_execution) {
            execute();
        }
    }
    
    Pipeline(Pipeline&& other) noexcept = default;
    Pipeline& operator=(Pipeline&& other) noexcept = default;
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    
    void execute() {
        step->execute();
    }
    
    void operator()() {
        execute();
    }
    
    // Для void пайплайна можно добавлять только терминальные операции
    template<typename F>
    auto operator|(F&& func) {
        struct SequentialStep : public PipelineStepBase {
            std::unique_ptr<PipelineStepBase> prev;
            std::function<void()> action;
            bool executed = false;
            
            SequentialStep(std::unique_ptr<PipelineStepBase> p, std::function<void()> f) 
                : prev(std::move(p)), action(f) {}
            
            void execute() override { 
                if (!executed) {
                    prev->execute();  
                    action();      
                    executed = true;
                }
            }
        };
        
        auto sequentialStep = std::make_unique<SequentialStep>(
            std::move(step), 
            [func = std::forward<F>(func)]() {
                func();
            }
        );
        
        return Pipeline<void>(std::move(sequentialStep), immediate_execution);
    }
};

template<typename T>
Pipeline<T> make_pipeline(T value, bool immediate = false) {
    auto initialStep = std::make_unique<InitialStep<T>>(value);
    return Pipeline<T>(std::move(initialStep), immediate);
}

struct SizeWrapper {
    template<typename T>
    auto operator()(const T& container) const {
        return std::size(container);
    }
};

SizeWrapper pipeline_size;

template<typename T, typename F>
auto operator|(T&& value, F&& func) {
    return make_pipeline(std::forward<T>(value), false) | std::forward<F>(func);
}

int main() {
    std::cout << "=== Test 1: Basic string pipeline ===" << std::endl;
    std::string str = "Hello World!";
    auto pipeline = str | pipeline_size 
                   | [](auto x){return x * 2;} 
                   | [](auto x){std::cout << "Result: " << x << std::endl;};
    pipeline();

    std::cout << "\n=== Test 2: Immediate execution ===" << std::endl;
    auto result2 = make_pipeline(std::string("Hello"), true) 
                  | pipeline_size 
                  | [](auto x){return x * 3;} 
                  | [](auto x){std::cout << "Immediate: " << x << std::endl;};

    std::cout << "\n=== Test 3: Number pipeline ===" << std::endl;
    auto numberPipeline = 5 | [](auto x){return x + 10;} 
                          | [](auto x){return x * x;} 
                          | [](auto x){std::cout << "Square: " << x << std::endl;};
    numberPipeline();

    std::cout << "\n=== Test 4: Complex pipeline ===" << std::endl;
    auto complexPipeline = std::string("Test") 
                          | pipeline_size
                          | [](auto x){return x + 100;} 
                          | [](auto x){return x / 2;} 
                          | [](auto x){std::cout << "Final: " << x << std::endl;};
    complexPipeline();

    std::cout << "\n=== Test 5: Step-by-step construction ===" << std::endl;
    auto intermediate = std::string("Pipeline") | pipeline_size | [](auto x){return x * 10;};
    auto finalPipeline = intermediate | [](auto x){std::cout << "Step-by-step: " << x << std::endl;};
    finalPipeline();

    std::cout << "\n=== Test 6: Multiple terminal operations ===" << std::endl;
    auto multiTerminal = make_pipeline(42, false) 
                        | [](auto x){std::cout << "First: " << x << std::endl;}
                        | [](){std::cout << "Second operation" << std::endl;};
    multiTerminal();

    std::cout << "\n=== Test 7: Simple terminal chain ===" << std::endl;
    auto test7 = make_pipeline(100, false) 
                | [](auto x){std::cout << "Value: " << x << std::endl;}
                | [](){std::cout << "Done!" << std::endl;};
    test7();

    std::cout << "\n=== Test 8: Vector pipeline ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto vecPipeline = vec | pipeline_size 
                      | [](auto x){return x * 10;} 
                      | [](auto x){std::cout << "Vector size * 10: " << x << std::endl;};
    vecPipeline();

    std::cout << "\n=== Test 9: List pipeline ===" << std::endl;
    std::list<double> lst = {1.1, 2.2, 3.3};
    auto listPipeline = lst | pipeline_size 
                       | [](auto x){return x + 5;} 
                       | [](auto x){std::cout << "List size + 5: " << x << std::endl;};
    listPipeline();

    std::cout << "\n=== Test 10: Array pipeline ===" << std::endl;
    std::array<char, 6> arr = {'a', 'b', 'c', 'd', 'e', 'f'};
    auto arrPipeline = arr | pipeline_size 
                      | [](auto x){return x - 1;} 
                      | [](auto x){std::cout << "Array size - 1: " << x << std::endl;};
    arrPipeline();

    std::cout << "\n=== Test 11: Multiple transformations ===" << std::endl;
    auto multiTransform = 2 
                         | [](auto x){return x * 3;} 
                         | [](auto x){return x + 7;} 
                         | [](auto x){return x / 2;} 
                         | [](auto x){return x * x;} 
                         | [](auto x){std::cout << "Multi-transform: " << x << std::endl;};
    multiTransform();

    std::cout << "\n=== Test 12: String transformations ===" << std::endl;
    auto strTransform = std::string("hello") 
                       | pipeline_size 
                       | [](auto x){return std::string("Size is: ") + std::to_string(x);} 
                       | [](auto str){std::cout << str << std::endl;};
    strTransform();

    std::cout << "\n=== Test 13: Lambda with capture ===" << std::endl;
    int multiplier = 5;
    auto capturePipeline = 10 
                          | [multiplier](auto x){return x * multiplier;} 
                          | [](auto x){std::cout << "Captured lambda: " << x << std::endl;};
    capturePipeline();

    std::cout << "\n=== Test 14: Multiple void operations ===" << std::endl;
    auto voidPipeline = make_pipeline(777, false) 
                       | [](auto x){std::cout << "Number: " << x << std::endl;} 
                       | [](){std::cout << "Step 1 completed" << std::endl;} 
                       | [](){std::cout << "Step 2 completed" << std::endl;} 
                       | [](){std::cout << "All done!" << std::endl;};
    voidPipeline();

    std::cout << "\n=== Test 15: Mixed types pipeline ===" << std::endl;
    auto mixedPipeline = std::string("ABCD") 
                        | pipeline_size 
                        | [](auto x){return static_cast<double>(x) * 1.5;} 
                        | [](auto x){return "Result: " + std::to_string(x);} 
                        | [](auto str){std::cout << str << std::endl;};
    mixedPipeline();

    std::cout << "\n=== Test 16: Direct execution ===" << std::endl;
    make_pipeline(999, true) 
        | [](auto x){std::cout << "Direct execution: " << x << std::endl;} 
        | [](){std::cout << "Direct execution completed" << std::endl;};

    std::cout << "\n=== Test 17: Another immediate execution ===" << std::endl;
    make_pipeline(std::vector<int>{1, 2, 3}, true)
        | pipeline_size
        | [](auto x){std::cout << "Vector size: " << x << std::endl;};

    std::cout << "\n=== All tests completed! ===" << std::endl;
    std::cin.get();
    return 0;
}
