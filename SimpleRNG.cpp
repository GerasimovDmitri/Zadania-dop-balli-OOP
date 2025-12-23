#ifndef SIMPLERNG_HPP
#define SIMPLERNG_HPP

#include <iterator>
#include <cmath>
#include <stdexcept>

class EndSentinel {
public:
    EndSentinel(double initial_x, double eps) : m_initial_x(initial_x), m_eps(eps) {}
    double initial_x() const { return m_initial_x; }
    double eps() const { return m_eps; }
private:
    double m_initial_x;
    double m_eps;
};

class SimpleRNG {
private:
    double m_a, m_c, m_m, m_current_x, m_initial_x;
    double m_eps;

    double next() {
        m_current_x = std::fmod(m_a * m_current_x + m_c, m_m);
        return m_current_x;
    }

public:
    SimpleRNG(double a, double c, double m, double start_x = 0.1, double eps_val = 0.05)
        : m_a(a), m_c(c), m_m(m), m_current_x(start_x), m_initial_x(start_x), m_eps(eps_val) {
        if (m_m <= 1.0 || m_a <= 0.0 || m_a >= 1.0 || m_c <= 0.0 || m_c >= m_m) {
            throw std::invalid_argument("Invalid parameters: m must be > 1, 0 < a < 1, 0 < c < m");
        }
    }

    void reset(double x) {
        m_current_x = x;
        m_initial_x = x;
    }

    void reset() {
        m_current_x = m_initial_x;
    }

    class Iterator {
    private:
        SimpleRNG* rng;
        double current_x;

    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = double;
        using difference_type = std::ptrdiff_t;
        using pointer = double*;
        using reference = double&;

        Iterator(SimpleRNG* r, double init_x) : rng(r), current_x(init_x) {}

        double operator*() const { return current_x; }

        Iterator& operator++() {
            current_x = rng->next();
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const EndSentinel& sent) const {
            return std::abs(current_x - sent.initial_x()) < sent.eps();
        }
        bool operator!=(const EndSentinel& sent) const { return !(*this == sent); }
    };

    Iterator begin() { return Iterator(this, m_current_x); }

    EndSentinel end(double eps = 0.05) {
        return EndSentinel(m_initial_x, eps);
    }
};

#endif
