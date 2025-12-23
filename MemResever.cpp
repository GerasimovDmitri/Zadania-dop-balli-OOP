#include <type_traits> 
#include <exception> 
#include <string>     

class NotEnoughSlotsError : public std::exception {
private:
    std::string msg_;
public:
    NotEnoughSlotsError(size_t created) {
        msg_ = "Недостаточно слотов. Текущих объектов: " + std::to_string(created);
    }
    const char* what() const noexcept override {
        return msg_.c_str();
    }
};

class EmptySlotError : public std::exception {
public:
    const char* what() const noexcept override {
        return "Объекта по этому номеру не существует";
    }
};

template <typename T, size_t N>
class MemReserver {
private:
    struct Slot {
        alignas(T) char data[sizeof(T)];
        bool occupied = false;

        T* obj() { return reinterpret_cast<T*>(data); }
        const T* obj() const { return reinterpret_cast<const T*>(data); }

        void construct(auto&&... args) {
            new (obj()) T(std::forward<decltype(args)>(args)...);
            occupied = true;
        }

        void destruct() {
            if (occupied) {
                obj()->~T();
                occupied = false;
            }
        }
    };

    Slot slots[N];
    size_t free_stack[N];
    size_t free_top = N;

public:
    MemReserver() {
        for (size_t i = 0; i < N; ++i) {
            free_stack[i] = i;
        }
    }

    ~MemReserver() {
        for (auto& s : slots) {
            s.destruct();
        }
    }

    template <typename... Args>
    T& create(Args&&... args) {
        if (free_top == 0) {
            throw NotEnoughSlotsError(count());
        }
        size_t idx = free_stack[--free_top];
        slots[idx].construct(std::forward<Args>(args)...);
        return *slots[idx].obj();
    }

    void delete_(size_t index) {
        if (index >= N || !slots[index].occupied) {
            throw EmptySlotError();
        }
        slots[index].destruct();
        free_stack[free_top++] = index;
    }

    size_t count() const {
        return N - free_top;
    }

    T& get(size_t index) {
        if (index >= N || !slots[index].occupied) {
            throw EmptySlotError();
        }
        return *slots[index].obj();
    }

    size_t position(const T& obj) {
        for (size_t i = 0; i < N; ++i) {
            if (slots[i].occupied && &obj == slots[i].obj()) {
                return i;
            }
        }
        throw EmptySlotError();
    }
};
