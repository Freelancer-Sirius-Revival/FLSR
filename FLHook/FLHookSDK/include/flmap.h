#pragma once

// Not quite a BST, through very similar.
template <typename KeyType, typename ValType>
class FlMap
{

public:
    struct Node;

    typedef Node* NodePtr;

    struct Node
    {
        NodePtr left;
        NodePtr parent;
        NodePtr right;
        KeyType key;
        ValType data;
    };

    class Iterator
    {
    public:
        Iterator(NodePtr currentNode, FlMap<KeyType, ValType>* classPtr)
        {
            this->currentNode = currentNode;
            classObj = classPtr;
        }

        Iterator()
        {
            currentNode = 0;
            classObj = 0;
        }

        void Inc()
        {
            if (!classObj->IsNil(currentNode->right))
            {
                currentNode = classObj->Min(currentNode->right);
            }
            else if (!classObj->IsNil(currentNode))
            { // climb looking for right subtree
                NodePtr node;
                while (!classObj->IsNil(node = currentNode->parent) && currentNode == node->right)
                {
                    currentNode = node; // ==> parent while right subtree
                }
                currentNode = node; // ==> parent (head if end())
            }

            // set to end node if we are at nil (so we can compare against end-iterator)
            if (classObj->IsNil(currentNode))
            {
                currentNode = classObj->end().currentNode;
            }
        }

        Iterator& operator++()
        {
            Inc();
            return *this;
        }
        Iterator operator++(int)
        {
            Iterator tmp(*this); // copy
            operator++();        // pre-increment
            return tmp;          // return old value
        }

        bool operator==(const Iterator& right) const
        { // test for iterator equality
            return (currentNode == right.currentNode);
        }

        bool operator!=(const Iterator& right) const
        { // test for iterator inequality
            return (!(*this == right));
        }

        unsigned int key() { return currentNode->key; }

        ValType* value() { return &currentNode->data; }

        NodePtr operator*() { return currentNode; }

    private:
        NodePtr currentNode;
        FlMap<KeyType, ValType>* classObj;
    };

public:
    unsigned int size() { return _size; };

    Iterator begin() { return Iterator(headNode->left, this); }

    Iterator end() { return Iterator(headNode, this); }

    Iterator find(KeyType key)
    {
        NodePtr searchNode = headNode->parent; // parent of headnode is first legit (upmost) node

        while (IsNil(searchNode) == false)
        {
            if (searchNode->key == key)
            {
                break;
            }

            if (key < searchNode->key)
            {
                searchNode = searchNode->left;
            }
            else
            {
                searchNode = searchNode->right;
            }
        }

        return Iterator(searchNode, this);
    }

protected:
    NodePtr Min(NodePtr node)
    {
        // go to leftmost child
        while (IsNil(node->left) == false)
        {
            node = node->left;
        }

        return node;
    }

    bool IsNil(NodePtr node) { return (node == endNode || node == headNode); }

private:
    void* dunno = nullptr;
    NodePtr headNode = nullptr; // headnode stores min/max in left/right and upmost node in parent
    NodePtr endNode = nullptr;
    void* dunno2 = nullptr;
    unsigned int _size = 0; // NOLINT
};