#include "dbms/core/LRU.h"

// TO_DO 
// flush page while deleted

namespace dbms::core{
// key is page_no uint32_t

Node::Node(uint32_t  key,void* value){
        this->key=key;
        this->value=value;
        next=nullptr;
        prev=nullptr;
}
Node:: ~Node(){
    if(this->next!=nullptr)delete this->next;
}

void LRUCache::insertStart(Node* newNode){
            newNode->prev=head;
            Node* nNode=head->next;
            newNode->next=nNode;
            nNode->prev=newNode;
            head->next=newNode;
}
void LRUCache::remove(Node* dNode,Node* tail){
        Node* pNode=dNode->prev;
        pNode->next=tail;
        tail->prev=pNode;
        dNode->prev=nullptr;
        dNode->next=nullptr;
}

LRUCache::LRUCache(uint32_t capacity) {
    this->capacity=capacity;
    head=new Node(0,nullptr);
    tail=new Node(0,nullptr);
    head->next=tail;
    tail->prev=head;
}

void* LRUCache::get(uint32_t  key) {
    if(m.find(key)!=m.end()){
        remove(m[key],m[key]->next);
        insertStart(m[key]);          
        return m[key]->value;
        }
    else return nullptr;
}

void LRUCache::put(uint32_t key, void* value) {
    if(m.find(key)!=m.end()){
        m[key]->value=value;
        remove(m[key],m[key]->next);
        insertStart(m[key]); 
        }
    else{                          
        Node* newNode=new Node(key,value);
        m[key]=newNode;
        if(count==capacity){                
            Node* dNode=tail->prev;
            remove(dNode,tail);
            m.erase(dNode->key);
            delete dNode;             
        }
        else{
            count++;
        }
        insertStart(newNode);

    }
}
void LRUCache::print_dirty(){
    Node* temp=head->next;
    bool check=true;            
    while(temp!=tail && GET_DIRTY(temp->value,PAGE_SIZE+1)){
        if(check){cout<<"Last Recently Used->";check=false;}
        cout<<"[ "<<GET_PAGE_NO(temp->value,true)<<" ]"<<endl;
        temp=temp->next;
    }
    if(check)cout<<"0 dirty pages!"<<endl;
}

}