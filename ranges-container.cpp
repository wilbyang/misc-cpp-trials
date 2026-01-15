#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <iterator>

// Node structure for singly-linked list
template<typename T>
struct Node {
    T data;
    Node* next;
    
    Node(const T& value) : data(value), next(nullptr) {}
};

// Forward declaration
template<typename T>
class LinkedList;

// Custom iterator for the linked list
template<typename T>
class LinkedListIterator {
private:
    Node<T>* current;
    
    friend class LinkedList<T>;
    
    explicit LinkedListIterator(Node<T>* node) : current(node) {}

public:
    // C++20 iterator traits
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    // Default constructor
    LinkedListIterator() : current(nullptr) {}

    // Dereference operator
    reference operator*() const {
        return current->data;
    }

    pointer operator->() const {
        return &(current->data);
    }

    // Pre-increment
    LinkedListIterator& operator++() {
        if (current) {
            current = current->next;
        }
        return *this;
    }

    // Post-increment
    LinkedListIterator operator++(int) {
        LinkedListIterator temp = *this;
        ++(*this);
        return temp;
    }

    // Equality comparison
    bool operator==(const LinkedListIterator& other) const {
        return current == other.current;
    }

    bool operator!=(const LinkedListIterator& other) const {
        return !(*this == other);
    }
};

// LinkedList container compatible with C++20 ranges
template<typename T>
class LinkedList {
private:
    Node<T>* head;
    Node<T>* tail;
    size_t count;

public:
    // Type aliases
    using value_type = T;
    using iterator = LinkedListIterator<T>;
    using const_iterator = LinkedListIterator<const T>;

    // Constructor
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}

    // Destructor
    ~LinkedList() {
        clear();
    }

    // Copy constructor
    LinkedList(const LinkedList& other) : head(nullptr), tail(nullptr), count(0) {
        for (const auto& item : other) {
            push_back(item);
        }
    }

    // Move constructor
    LinkedList(LinkedList&& other) noexcept 
        : head(other.head), tail(other.tail), count(other.count) {
        other.head = nullptr;
        other.tail = nullptr;
        other.count = 0;
    }

    // Copy assignment
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            for (const auto& item : other) {
                push_back(item);
            }
        }
        return *this;
    }

    // Move assignment
    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            head = other.head;
            tail = other.tail;
            count = other.count;
            other.head = nullptr;
            other.tail = nullptr;
            other.count = 0;
        }
        return *this;
    }

    // Range interface - KEY for C++20 ranges compatibility
    iterator begin() {
        return iterator(head);
    }

    iterator end() {
        return iterator(nullptr);
    }

    const_iterator begin() const {
        return const_iterator(reinterpret_cast<Node<const T>*>(head));
    }

    const_iterator end() const {
        return const_iterator(nullptr);
    }

    const_iterator cbegin() const {
        return begin();
    }

    const_iterator cend() const {
        return end();
    }

    // Container operations
    void push_back(const T& value) {
        Node<T>* new_node = new Node<T>(value);
        if (!head) {
            head = tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
        ++count;
    }

    void push_front(const T& value) {
        Node<T>* new_node = new Node<T>(value);
        if (!head) {
            head = tail = new_node;
        } else {
            new_node->next = head;
            head = new_node;
        }
        ++count;
    }

    bool empty() const {
        return count == 0;
    }

    size_t size() const {
        return count;
    }

    void clear() {
        while (head) {
            Node<T>* temp = head;
            head = head->next;
            delete temp;
        }
        tail = nullptr;
        count = 0;
    }

    T& front() {
        return head->data;
    }

    const T& front() const {
        return head->data;
    }
};

// Demo function to show various range operations
void demonstrate_ranges() {
    std::cout << "C++20 Ranges-Compatible Custom Container Demo\n";
    std::cout << "=============================================\n\n";

    // Create and populate the linked list
    LinkedList<int> list;
    std::cout << "Creating LinkedList and adding elements: ";
    for (int i = 1; i <= 10; ++i) {
        list.push_back(i);
        std::cout << i << " ";
    }
    std::cout << "\n\n";

    // 1. Range-based for loop (basic range requirement)
    std::cout << "1. Range-based for loop:\n   ";
    for (auto x : list) {
        std::cout << x << " ";
    }
    std::cout << "\n\n";

    // 2. std::ranges::for_each
    std::cout << "2. std::ranges::for_each (print squares):\n   ";
    std::ranges::for_each(list, [](int x) {
        std::cout << x * x << " ";
    });
    std::cout << "\n\n";

    // 3. std::ranges::count_if
    auto even_count = std::ranges::count_if(list, [](int x) {
        return x % 2 == 0;
    });
    std::cout << "3. Count even numbers: " << even_count << "\n\n";

    // 4. std::ranges::find
    auto it = std::ranges::find(list, 7);
    if (it != list.end()) {
        std::cout << "4. Found value 7 in the list\n\n";
    }

    // 5. std::ranges::any_of / all_of / none_of
    bool has_greater_than_5 = std::ranges::any_of(list, [](int x) {
        return x > 5;
    });
    std::cout << "5. Any element > 5? " << (has_greater_than_5 ? "Yes" : "No") << "\n\n";

    // 6. std::ranges::max_element
    auto max_it = std::ranges::max_element(list);
    if (max_it != list.end()) {
        std::cout << "6. Maximum element: " << *max_it << "\n\n";
    }

    // 7. std::accumulate (works with iterators)
    auto sum = std::accumulate(list.begin(), list.end(), 0);
    std::cout << "7. Sum of all elements: " << sum << "\n\n";

    // 8. std::views::filter (range adaptors)
    std::cout << "8. Filter even numbers using views:\n   ";
    auto even_view = list | std::views::filter([](int x) {
        return x % 2 == 0;
    });
    for (auto x : even_view) {
        std::cout << x << " ";
    }
    std::cout << "\n\n";

    // 9. std::views::transform
    std::cout << "9. Transform (multiply by 10) using views:\n   ";
    auto transformed = list | std::views::transform([](int x) {
        return x * 10;
    });
    for (auto x : transformed) {
        std::cout << x << " ";
    }
    std::cout << "\n\n";

    // 10. Chained views (filter + transform)
    std::cout << "10. Chained views (filter > 5, then square):\n    ";
    auto chained = list 
        | std::views::filter([](int x) { return x > 5; })
        | std::views::transform([](int x) { return x * x; });
    for (auto x : chained) {
        std::cout << x << " ";
    }
    std::cout << "\n\n";

    // 11. std::views::take
    std::cout << "11. Take first 5 elements:\n    ";
    auto first_five = list | std::views::take(5);
    for (auto x : first_five) {
        std::cout << x << " ";
    }
    std::cout << "\n\n";

    // 12. std::views::drop
    std::cout << "12. Drop first 5 elements:\n    ";
    auto last_five = list | std::views::drop(5);
    for (auto x : last_five) {
        std::cout << x << " ";
    }
    std::cout << "\n\n";

    // 13. std::ranges::copy
    std::cout << "13. Copy to vector using std::ranges::copy:\n    ";
    std::vector<int> vec;
    std::ranges::copy(list, std::back_inserter(vec));
    for (auto x : vec) {
        std::cout << x << " ";
    }
    std::cout << "\n\n";
}

int main() {
    demonstrate_ranges();

    std::cout << "\nKey Takeaways:\n";
    std::cout << "- Custom container works seamlessly with C++20 ranges\n";
    std::cout << "- Only need: begin(), end(), and proper iterator implementation\n";
    std::cout << "- Iterator must satisfy forward_iterator concept\n";
    std::cout << "- Enables all std::ranges algorithms and views\n";
    std::cout << "- Supports range-based for loops and view pipelines\n";

    return 0;
}
