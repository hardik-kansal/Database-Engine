#ifndef LRU_H
#define LRU_H
#include "headerfiles.h"
#include "structs.h"
#include "utils.h"
using namespace std;

// TO_DO 
// flush page while deleted

// key is page_no uint32_t
struct Node{
    uint32_t key;
    void* value;
    Node* next;
    Node* prev;
    Node(uint32_t  key,void* value){
        this->key=key;
        this->value=value;
        next=nullptr;
        prev=nullptr;
    }
    ~Node(){
        if(this->next!=nullptr)delete this->next;
    }
};
class LRUCache {
public:
        uint32_t  capacity;
        uint32_t  count=0;
        unordered_map<uint32_t ,Node*> m;
        Node* head;
        Node* tail;
        uint32_t salt1;
        uint32_t salt2;
        uint64_t checkMagic;
        uint16_t no_of_pages_in_journal;


        void insertStart(Node* newNode){
                    newNode->prev=head;
                    Node* nNode=head->next;
                    newNode->next=nNode;
                    nNode->prev=newNode;
                    head->next=newNode;
        }
        void remove(Node* dNode,Node* tail){
                Node* pNode=dNode->prev;
                pNode->next=tail;
                tail->prev=pNode;
                dNode->prev=nullptr;
                dNode->next=nullptr;
        }
    
        LRUCache(uint32_t capacity) {
            this->capacity=capacity;
            head=new Node(0,nullptr);
            tail=new Node(0,nullptr);
            head->next=tail;
            tail->prev=head;
        }
        
        void* get(uint32_t  key) {
            if(m.find(key)!=m.end()){
                remove(m[key],m[key]->next);
                insertStart(m[key]);          
                return m[key]->value;
                }
            else return nullptr;
        }
        
        void put(uint32_t key, void* value) {
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
        void print_dirty(){
            Node* temp=head->next;
            bool check=true;            
            while(temp!=tail && GET_DIRTY(temp->value,PAGE_SIZE+1)){
                if(check){cout<<"Last Recently Used->";check=false;}
                cout<<"[ "<<GET_PAGE_NO(temp->value,true)<<" ]"<<endl;
                temp=temp->next;
            }
            if(check)cout<<"0 dirty pages!"<<endl;
        }
    };
    


#endif