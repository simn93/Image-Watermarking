#ifndef ASSIGNMENT_SPM_BLOCKING_QUEUE_H
#define ASSIGNMENT_SPM_BLOCKING_QUEUE_H

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>

// simple blocking queue
template <typename T>
class BlockingQueue {
    private:
        std::mutex              d_mutex;
        std::condition_variable d_condition;
        std::deque<T>           d_queue;
    public:
        BlockingQueue(){}

        void push(T const& value) {
            {
                std::unique_lock<std::mutex> lock(this->d_mutex);
                d_queue.push_front(value);
            }
            this->d_condition.notify_one();
        }

        void push(std::vector<T> vector){
            for(T const& value: vector){
                this->push(value);
            }
        }

        T pop() {
            std::unique_lock<std::mutex> lock(this->d_mutex);
            this->d_condition.wait(lock, [=]{ return this->d_queue.size() > 0; });

            T rc(std::move(this->d_queue.back()));
            this->d_queue.pop_back();
            return rc;
        }

        bool isEmpty(){
           std::unique_lock<std::mutex> lock(this->d_mutex);
           return d_queue.empty();
        }

        int size(){
            std::unique_lock<std::mutex> lock(this->d_mutex);
            return (int)d_queue.size();
        }
};

#endif