#include <iostream>
#include <vector>
#include <iterator>
#include <cmath>
#include <stdexcept>

class SimpleRNG {
private:
    int m_m;
    double m_a, m_c;
    double m_current_x;
public:
    SimpleRNG(int m, double a, double c, double initial_x = 0.0)
        : m_m(m), m_a(a), m_c(c), m_current_x(initial_x) {
        if (m <= 1 || a <= 0 || a >= 1 || c <= 0 || c >= m) {
            throw std::invalid_argument("Invalid parameters, Peredekivai");
        }
    }

    void reset(double x) {
        m_current_x = x;
    }

    void reset() {
        m_current_x = 0.0;
    }

    class Iterator {
    private:
        int m_m;
        double m_a, m_c;
        double m_current_x;
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = double;
        using difference_type = ptrdiff_t;
        using pointer = double*;
        using reference = double&; 

        Iterator(int m, double a, double c, double x)
            : m_m(m), m_a(a), m_c(c), m_current_x(x) {}

        value_type operator*() const {
            return m_current_x;
        }

        Iterator& operator++() {
            m_current_x = std::fmod(m_a * m_current_x + m_c, static_cast<double>(m_m));
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }
    };

    class EndSentinel {
        friend SimpleRNG;
    private:
        double initial_x;
        double eps;
        EndSentinel(double i, double e) : initial_x(i), eps(e) {}
    public:
        bool operator==(const EndSentinel& other) const {
            return initial_x == other.initial_x && eps == other.eps;
        }
        bool operator!=(const EndSentinel& other) const {
            return !(*this == other);
        }
    };

    Iterator begin() const {
        return Iterator(m_m, m_a, m_c, m_current_x);
    }

    EndSentinel end(double eps = 0.05) {
        return EndSentinel(m_current_x, eps);
    }

    friend bool operator==(const Iterator& it, const EndSentinel& sent) {
        return std::abs(it.m_current_x - sent.initial_x) < sent.eps;
    }

    friend bool operator==(const EndSentinel& sent, const Iterator& it) {
        return it == sent;
    }

    friend bool operator!=(const Iterator& it, const EndSentinel& sent) {
        return !(it == sent);
    }

    friend bool operator!=(const EndSentinel& sent, const Iterator& it) {
        return !(it == sent);
    }
};
