#include "qobject.hpp"
#include <cstdio>
#include <cstdlib>

#define Q_CHECK(condition) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

class TestObject : public QObject {
public:
    TestObject(QObject *parent = nullptr) : QObject(parent) {}
};

bool testParentDeletesChild() {
    TestObject *parent = new TestObject;
    QPointer<TestObject> child = new TestObject(parent);
    Q_CHECK(child != nullptr);
    delete parent;
    Q_CHECK(child.isNull());
    return true;
}

bool testChildDeletesSelf() {
    TestObject parent;
    {
        QPointer<TestObject> child = new TestObject(&parent);
        Q_CHECK(parent.hasChildren());
        delete child.get();
    }
    Q_CHECK(!parent.hasChildren());
    return true;
}

bool testReparenting() {
    TestObject *parent1 = new TestObject;
    TestObject *parent2 = new TestObject;
    QPointer<TestObject> child = new TestObject(parent1);
    Q_CHECK(child->parent() == parent1);
    child->setParent(parent2);
    Q_CHECK(child->parent() == parent2);
    delete parent1;
    Q_CHECK(!child.isNull());
    delete parent2;
    Q_CHECK(child.isNull());
    return true;
}

bool testQPointerWithParentChild() {
    TestObject *parent = new TestObject;
    QPointer<TestObject> child = new TestObject(parent);
    QPointer<TestObject> grandChild = new TestObject(child);
    delete parent;
    Q_CHECK(child.isNull());
    Q_CHECK(grandChild.isNull());
    return true;
}

int main() {
    bool allTestsPassed = true;
    allTestsPassed &= testParentDeletesChild();
    allTestsPassed &= testChildDeletesSelf();
    allTestsPassed &= testReparenting();
    allTestsPassed &= testQPointerWithParentChild();

    if (allTestsPassed) {
        printf("All parent-child tests passed!\n");
    } else {
        printf("Some parent-child tests failed.\n");
    }
    return 0;
}