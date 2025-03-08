#pragma once
#include <cassert>
#include <cstddef>

#define Q_ASSERT(cond) assert(cond)

#define Q_DECLARE_PRIVATE(Class) \
    Class##Private *d_func() { return static_cast<Class##Private*>(d_ptr); } \
    const Class##Private *d_func() const { return static_cast<const Class##Private*>(d_ptr); } \
    friend struct Class##Private;

class QObject;

struct QWeakPointerData {
    QObject *object = nullptr;
    QWeakPointerData *next = nullptr;
    QWeakPointerData *prev = nullptr;

    QWeakPointerData() = default;
    QWeakPointerData(const QWeakPointerData&) = delete;
    QWeakPointerData& operator=(const QWeakPointerData&) = delete;
};

struct QObjectPrivate {
    QObject *q_ptr = nullptr;
    QObject *parent = nullptr;
    QObject *childrenHead = nullptr;
    QWeakPointerData *weak_pointers = nullptr;

    void addWeakPointer(QWeakPointerData **data);
    void removeWeakPointer(QWeakPointerData **data);
};

class QObject {
    Q_DECLARE_PRIVATE(QObject)
    template <typename> friend class QPointer;
public:
    explicit QObject(QObject *parent = nullptr);
    virtual ~QObject();

    void setParent(QObject *newParent);
    QObject* parent() const { return d_ptr->parent; }

    void addChild(QObject *child);
    void removeChild(QObject *child);

    bool hasChildren() const { return d_ptr->childrenHead != nullptr; }

private:
    void cleanupChildren();

    QObject *nextSibling = nullptr;
    QObject *prevSibling = nullptr;
    QObjectPrivate *d_ptr;
};

QObject::QObject(QObject *parent) : d_ptr(new QObjectPrivate) {
    d_ptr->q_ptr = this;
    setParent(parent);
}

QObject::~QObject() {
    if (d_ptr->parent) {
        d_ptr->parent->removeChild(this);
    }

    QWeakPointerData *current = d_ptr->weak_pointers;
    while (current) {
        QWeakPointerData *next = current->next;
        current->object = nullptr;
        if (current->prev) {
            current->prev->next = current->next;
        } else {
            d_ptr->weak_pointers = current->next;
        }
        if (current->next) {
            current->next->prev = current->prev;
        }
        current->prev = current->next = nullptr;
        current = next;
    }

    cleanupChildren();
    delete d_ptr;
}

void QObjectPrivate::addWeakPointer(QWeakPointerData **data) {
    if (!*data) {
        *data = new QWeakPointerData;
        (*data)->prev = nullptr;
        (*data)->next = weak_pointers;
        if (weak_pointers) {
            weak_pointers->prev = *data;
        }
        weak_pointers = *data;
    }
    (*data)->object = q_ptr;
}

void QObjectPrivate::removeWeakPointer(QWeakPointerData **data) {
    if (!*data) return;
    QWeakPointerData *current = *data;
    if (current->object) {
        if (current->prev) {
            current->prev->next = current->next;
        } else {
            current->object->d_ptr->weak_pointers = current->next;
        }
        if (current->next) {
            current->next->prev = current->prev;
        }
    }
    current->object = nullptr;
    delete current;
    *data = nullptr;
}

void QObject::setParent(QObject *newParent) {
    if (d_ptr->parent == newParent) return;

    if (d_ptr->parent) {
        d_ptr->parent->removeChild(this);
    }

    d_ptr->parent = newParent;

    if (newParent) {
        newParent->addChild(this);
    }
}

void QObject::addChild(QObject *child) {
    Q_ASSERT(child->d_ptr->parent == this);
    child->nextSibling = d_ptr->childrenHead;
    if (d_ptr->childrenHead) {
        d_ptr->childrenHead->prevSibling = child;
    }
    d_ptr->childrenHead = child;
}

void QObject::removeChild(QObject *child) {
    if (d_ptr->childrenHead == child) {
        d_ptr->childrenHead = child->nextSibling;
    }
    if (child->prevSibling) {
        child->prevSibling->nextSibling = child->nextSibling;
    }
    if (child->nextSibling) {
        child->nextSibling->prevSibling = child->prevSibling;
    }
    child->prevSibling = child->nextSibling = nullptr;
}

void QObject::cleanupChildren() {
    QObject *child = d_ptr->childrenHead;
    while (child) {
        QObject *nextChild = child->nextSibling;
        delete child;
        child = nextChild;
    }
    d_ptr->childrenHead = nullptr;
}

template <typename T>
class QPointer {
    QWeakPointerData *d = nullptr;

public:
    QPointer() = default;
    QPointer(T *obj) : d(nullptr) { 
        if (obj) {
            obj->d_func()->addWeakPointer(reinterpret_cast<QWeakPointerData**>(&d));
        }
    }
    QPointer(const QPointer &other) : d(nullptr) {
        if (other.d && other.d->object) {
            other.d->object->d_func()->addWeakPointer(reinterpret_cast<QWeakPointerData**>(&d));
        }
    }
    ~QPointer() { reset(); }

    QPointer &operator=(const QPointer &other) {
        if (this != &other) {
            reset();
            if (other.d && other.d->object) {
                other.d->object->d_func()->addWeakPointer(reinterpret_cast<QWeakPointerData**>(&d));
            }
        }
        return *this;
    }

    QPointer &operator=(T *obj) {
        reset();
        if (obj) {
            obj->d_func()->addWeakPointer(reinterpret_cast<QWeakPointerData**>(&d));
        }
        return *this;
    }

    void reset() {
        if (d) {
            if (d->object) {
                d->object->d_func()->removeWeakPointer(reinterpret_cast<QWeakPointerData**>(&d));
            } else {
                delete d;
                d = nullptr;
            }
        }
    }

    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    operator T*() const { return get(); }
    T* get() const { return d ? static_cast<T*>(d->object) : nullptr; }
    bool isNull() const { return get() == nullptr; }

    template <typename X>
    friend bool comparesEqual(const QPointer& lhs, const QPointer<X>& rhs) noexcept {
        return lhs.get() == rhs.get();
    }

    template <typename X>
    friend bool comparesEqual(const QPointer& lhs, X* rhs) noexcept {
        return lhs.get() == rhs;
    }

    friend bool comparesEqual(const QPointer& lhs, std::nullptr_t rhs) noexcept {
        return lhs.get() == rhs;
    }
};

template <typename T, typename X>
bool operator==(const QPointer<T>& lhs, const QPointer<X>& rhs) noexcept {
    return comparesEqual(lhs, rhs);
}

template <typename T, typename X>
bool operator!=(const QPointer<T>& lhs, const QPointer<X>& rhs) noexcept {
    return !comparesEqual(lhs, rhs);
}

template <typename T, typename X>
bool operator==(const QPointer<T>& lhs, X* rhs) noexcept {
    return comparesEqual(lhs, rhs);
}

template <typename T, typename X>
bool operator!=(const QPointer<T>& lhs, X* rhs) noexcept {
    return !comparesEqual(lhs, rhs);
}

template <typename X, typename T>
bool operator==(X* lhs, const QPointer<T>& rhs) noexcept {
    return comparesEqual(rhs, lhs);
}

template <typename X, typename T>
bool operator!=(X* lhs, const QPointer<T>& rhs) noexcept {
    return !comparesEqual(rhs, lhs);
}

template <typename T>
bool operator==(const QPointer<T>& lhs, std::nullptr_t rhs) noexcept {
    return comparesEqual(lhs, rhs);
}

template <typename T>
bool operator!=(const QPointer<T>& lhs, std::nullptr_t rhs) noexcept {
    return !comparesEqual(lhs, rhs);
}

template <typename T>
bool operator==(std::nullptr_t lhs, const QPointer<T>& rhs) noexcept {
    return comparesEqual(rhs, lhs);
}

template <typename T>
bool operator!=(std::nullptr_t lhs, const QPointer<T>& rhs) noexcept {
    return !comparesEqual(rhs, lhs);
}
