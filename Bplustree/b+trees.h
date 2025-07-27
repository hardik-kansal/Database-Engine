
#include<bits/stdc++.h>
#include <chrono>
#include <fstream>

using namespace std;
using namespace std::chrono;
#ifndef Bplustree
#define Bplustree
template <typename T>
struct Node{
    bool isLeaf;
    vector<int> keys;
    vector<Node<T>*> children;
    Node<T>* next; 
    vector<T> pos;
    Node<T>(bool isLeaf){
        next=nullptr;
        this->isLeaf=isLeaf;
    }
};

template <typename T>
class Bplustrees{
    private:
    Node<T>* root=nullptr;
    uint32_t M=3;
    uint32_t MAX_KEYS=M-1;
    uint32_t MIN_KEYS = ceil((M - 1) / 2.0);
    int ub(Node<T>* curr,int key){
        return upper_bound(curr->keys.begin(),curr->keys.end(),key)-curr->keys.begin();
    }
    int lb(Node<T>* curr,int key){
        return lower_bound(curr->keys.begin(),curr->keys.end(),key)-curr->keys.begin();
    }
    void fixInternalUnderflow(vector<pair<Node<T>*, int>>& path) {
        while (!path.empty()) {
            auto [node, idxInParent] = path.back();
            path.pop_back();
    
            
            if (node == root) {
                if (!node->isLeaf && node->keys.empty()) {
                    root = node->children[0];
                    delete node;
                }
                return;
            }
    
            Node<T>* parent = path.back().first;
            int indexInParent = path.back().second;
    
            Node<T>* leftSibling = (indexInParent > 0) ? parent->children[indexInParent - 1] : nullptr;
            Node<T>* rightSibling = (indexInParent + 1 < parent->children.size()) ? parent->children[indexInParent + 1] : nullptr;
    
    
            
            if (leftSibling && leftSibling->keys.size() > MIN_KEYS) {
                
                node->keys.insert(node->keys.begin(), parent->keys[indexInParent - 1]);
                parent->keys[indexInParent - 1] = leftSibling->keys.back();
                leftSibling->keys.pop_back();
    
                
                if (!leftSibling->isLeaf) {
                    node->children.insert(node->children.begin(), leftSibling->children.back());
                    leftSibling->children.pop_back();
                }
                return;
            }
    
            
            if (rightSibling && rightSibling->keys.size() > MIN_KEYS) {
                
                node->keys.push_back(parent->keys[indexInParent]);
                parent->keys[indexInParent] = rightSibling->keys.front();
                rightSibling->keys.erase(rightSibling->keys.begin());
    
                
                if (!rightSibling->isLeaf) {
                    node->children.push_back(rightSibling->children.front());
                    rightSibling->children.erase(rightSibling->children.begin());
                }
                return;
            }
    
            
            if (leftSibling) {
                leftSibling->keys.push_back(parent->keys[indexInParent - 1]);
                leftSibling->keys.insert(leftSibling->keys.end(), node->keys.begin(), node->keys.end());
    
                if (!node->isLeaf) {
                    leftSibling->children.insert(leftSibling->children.end(), node->children.begin(), node->children.end());
                }
    
                parent->keys.erase(parent->keys.begin() + indexInParent - 1);
                parent->children.erase(parent->children.begin() + indexInParent);
    
                delete node;
            }
            
            else if (rightSibling) {
                node->keys.push_back(parent->keys[indexInParent]);
                node->keys.insert(node->keys.end(), rightSibling->keys.begin(), rightSibling->keys.end());
    
                if (!node->isLeaf) {
                    node->children.insert(node->children.end(), rightSibling->children.begin(), rightSibling->children.end());
                }
    
                parent->keys.erase(parent->keys.begin() + indexInParent);
                parent->children.erase(parent->children.begin() + indexInParent + 1);
    
                delete rightSibling;
            }
    
            
            node = parent;
        }
    }
    
    void fixLeafUnderflow(vector<pair<Node<T>*,int>>& path) {
        Node<T>* leaf = path.back().first;
        Node<T>* parent = path[path.size() - 2].first;
    
        int idx = -1;
        for (int i = 0; i < parent->children.size(); i++) {
            if (parent->children[i] == leaf) {
                idx = i;
                break;
            }
        }
    
        
        if (idx > 0) {
            Node<T>* left = parent->children[idx - 1];
            if (left->keys.size() > MIN_KEYS) {
                
                leaf->keys.insert(leaf->keys.begin(), left->keys.back());
                leaf->pos.insert(leaf->pos.begin(), left->pos.back());
                left->keys.pop_back();
                left->pos.pop_back();
                parent->keys[idx - 1] = leaf->keys[0];
                return;
            }
        }
    
        
        if (idx < parent->children.size() - 1) {
            Node<T>* right = parent->children[idx + 1];
            if (right->keys.size() > MIN_KEYS) {
                
                leaf->keys.push_back(right->keys.front());
                leaf->pos.push_back(right->pos.front());
                right->keys.erase(right->keys.begin());
                right->pos.erase(right->pos.begin());
                parent->keys[idx] = right->keys[0];
                return;
            }
        }
    
        
        if (idx > 0) {
            Node<T>* left = parent->children[idx - 1];
            left->keys.insert(left->keys.end(), leaf->keys.begin(), leaf->keys.end());
            left->pos.insert(left->pos.end(), leaf->pos.begin(), leaf->pos.end());
            left->next = leaf->next;
    
            parent->children.erase(parent->children.begin() + idx);
            parent->keys.erase(parent->keys.begin() + idx - 1);
            delete leaf;
        }
        
        else if (idx < parent->children.size() - 1) {
            Node<T>* right = parent->children[idx + 1];
            leaf->keys.insert(leaf->keys.end(), right->keys.begin(), right->keys.end());
            leaf->pos.insert(leaf->pos.end(), right->pos.begin(), right->pos.end());
            leaf->next = right->next;
    
            parent->children.erase(parent->children.begin() + idx + 1);
            parent->keys.erase(parent->keys.begin() + idx);
            delete right;
        }
    
       
        if (parent == root && parent->keys.empty()) {
            root = parent->children[0];
            delete parent;
            return;
        }
    
        
        if (parent->keys.size() < MIN_KEYS && path.size() > 2) {
            path.pop_back();  // remove leaf
            fixInternalUnderflow(path);
        }
    }
    
    void insertInternal(vector<Node<T>*>& path, int key, Node<T>* rightChild) {
        if (path.size() == 1) {
            
            Node<T>* oldRoot = path[0];
            Node<T>* newRoot = new Node<T>(false);
            newRoot->keys.push_back(key);
            newRoot->children.push_back(oldRoot);
            newRoot->children.push_back(rightChild);
            root = newRoot;
            return;
        }
        for (int level = path.size() - 2; level >= 0; --level) {
            Node<T>* parent = path[level];
    
            int idx = ub(parent,key);
            parent->keys.insert(parent->keys.begin() + idx, key);
            parent->children.insert(parent->children.begin() + idx + 1, rightChild);
    
            if (parent->keys.size() <= MAX_KEYS)
                return;
    
            
            Node<T>* newInternal = new Node<T>(false);
            int mid = (parent->keys.size()) / 2;
    
            newInternal->keys.assign(parent->keys.begin() + mid + 1, parent->keys.end());
            newInternal->children.assign(parent->children.begin() + mid + 1, parent->children.end());
    
            int upKey = parent->keys[mid];
    
            parent->keys.resize(mid);
            parent->children.resize(mid + 1);
    
            if (level == 0) {
                
                Node<T>* newRoot = new Node<T>(false);
                newRoot->keys.push_back(upKey);
                newRoot->children.push_back(parent);
                newRoot->children.push_back(newInternal);
                root = newRoot;
            } else {
                rightChild = newInternal;
                key = upKey;
                continue; 
            }
    
            break;
        }
    }
    
    public:
    Bplustrees(int M){
        root = new Node<T>(true);
        this->M=M;
        this->MAX_KEYS=M-1;
        this->MIN_KEYS = ceil((M - 1) / 2.0);
    }
    T* search(int key){
        Node<T>* curr=root;
        while(!curr->isLeaf){
            int index=ub(curr,key);
            curr=curr->children[index];
        }
        int index=lb(curr,key);
        return (index<curr->keys.size() && curr->keys[index]==key) ? &(curr->pos[index]) : nullptr;
    }
    void insert(int key,T pos){
        vector<Node<T>*> path;
        Node<T>* curr = root;
        path.push_back(curr);

        while (!curr->isLeaf) {
            int idx = ub(curr,key);
            curr = curr->children[idx];
            path.push_back(curr);
        }
        int idx = lb(curr,key);
        if(idx<curr->keys.size() && curr->keys[idx]==key){curr->pos[idx]=pos;return;}
        curr->keys.insert(curr->keys.begin() + idx, key);
        curr->pos.insert(curr->pos.begin() + idx, pos);
        if (curr->keys.size() > MAX_KEYS) {
            Node<T>* newLeaf = new Node<T>(true);

            int mid = (curr->keys.size() + 1) / 2;
            newLeaf->keys.assign(curr->keys.begin() + mid, curr->keys.end());
            newLeaf->pos.assign(curr->pos.begin() + mid, curr->pos.end());

            curr->keys.resize(mid);
            curr->pos.resize(mid);

            newLeaf->next = curr->next;
            curr->next = newLeaf;
            insertInternal(path, newLeaf->keys[0], newLeaf);
        }

    }
    void deleteKey(int key) {
        vector<pair<Node<T>*,int>> path;
        Node<T>* curr = root;
        path.push_back({root,-1});
    
        
        while (!curr->isLeaf) {
            int idx = ub(curr, key);
            curr = curr->children[idx];
            path.push_back({curr,idx});
        }
    
        
        int idx = lb(curr, key);
        if (idx >= curr->keys.size() || curr->keys[idx] != key) {
            cout << "Key not found\n";
            return;
        }
    
       
        curr->keys.erase(curr->keys.begin() + idx);
        curr->pos.erase(curr->pos.begin() + idx);

        if(root->keys[0]==key)root->keys[0]=curr->keys[0];
        
        if (curr == root || curr->keys.size() >= MIN_KEYS)
            return;
    
        fixLeafUnderflow(path);
        if(root->keys[0]==key)root->keys[0]=curr->keys[0];


    }
    void printNode(Node<T>* node){
        if(node==nullptr)return;
        for(auto &val:node->keys){
            cout<<val<<' ';
        }
    }
    void printTree() {
        Node<T>* curr = root;
        queue<Node<T>*> q;
        q.push(curr);
        q.push(nullptr);
        while (!q.empty()) {
            Node<T>* node = q.front();
            q.pop();
            if (node == nullptr) {
                cout << endl;
                if (!q.empty()) q.push(nullptr);
            } else {
                printNode(node);
                cout << "  ";
                for (auto val : node->children) {
                    q.push(val);
                }
            }
        }
    }
};
#endif

// template <typename T>
// void runBenchmark(ofstream& fout,int m, int n) {
//     Bplustrees<T> tree(m);
//     T val{};
//     auto start = high_resolution_clock::now();

//     for (int i = 0; i < n; ++i) {
    
//         tree.insert(i,val);
//     }

//     auto end = high_resolution_clock::now();
//     auto duration = duration_cast<milliseconds>(end - start);

//     fout << m << "," << n << "," << duration.count()<< endl;
// }


// int main(){

//     ofstream fout("bplus_benchmark.csv");
//     fout << "m,n,time_ms\n"; 

//     for (int m=3;m<100000;m=m+1000) {
//         for (int n=3;n<100000;n=n+1000) {
//             runBenchmark<int>(fout, m, n);
//         }
//     }

//     fout.close();
//     return 0;

// }