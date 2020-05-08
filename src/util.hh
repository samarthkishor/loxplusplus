#ifndef __UTIL_H_
#define __UTIL_H_

#include <stack>
#include <vector>

/**
 * Copies the contents of stack a into stack b
 */
template <typename T>
void copyStack(std::stack<T>& a, std::stack<T>& b) {
    std::vector<T> vec;
    while (!a.empty()) {
        T top = a.top();
        a.pop();
        vec.push_back(top);
    }

    for (auto i = vec.rbegin(); i != vec.rend(); i++) {
        a.push(*i);
        b.push(*i);
    }
}

/**
 * Copies the contents of stack a into stack b in reverse order
 */
template <typename T>
void reverseStack(std::stack<T>& a, std::stack<T>& b) {
    std::stack<T> temp;
    copyStack(a, temp);
    while (!temp.empty()) {
        auto top = temp.top();
        temp.pop();
        b.push(top);
    }
}

#endif  // __UTIL_H_
