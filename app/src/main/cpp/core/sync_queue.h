/**
 * description:   <br>
 * @author 秦城季
 * @date 2020/11/11
 */
#include <queue>
#include <pthread.h>

using namespace std;

template<typename T>
class SyncQueue {
    /*typedef的功能是定义新的类型。
     * 第一句就是定义了一种pfun的类型，
     * 并定义这种类型为指向某种函数的指针，
     * 这种函数以一个int为参数并返回void类型。*/
    typedef void (*ReleaseCallback)(T &);

public:
    SyncQueue() {
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
    }

    ~SyncQueue() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    int push(T t) {
        pthread_mutex_lock(&mutex);
        if (work) {
            datas.push(t);
            pthread_cond_signal(&cond);
        } else {
            //不允许为空，否者会造成内存泄露
            releaseCallback(t);
        }
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    /**
     * @param t 传递一个指针进来，然后给该指针指向一个值
     * @return 0为失败
     */
    int pop(T &t) {
        int ret = 0;
        pthread_mutex_lock(&mutex);
        //在多核处理器下 由于竞争可能虚假唤醒 包括jdk也说明了
        while (work && datas.empty()){
            pthread_cond_wait(&cond,&mutex);
        }
        if(!datas.empty()){
            t = datas.front();
            datas.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&mutex);
        return ret;
    }

    void setReleaseCallback(ReleaseCallback release) {
        this->releaseCallback = release;
    }

    void setWork(int w){
        pthread_mutex_lock(&mutex);
        this->work = w;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    void clear(){
        pthread_mutex_lock(&mutex);
        while (!datas.empty()){
            T t = datas.front();
            datas.pop();
            releaseCallback(t);
        }
        pthread_mutex_unlock(&mutex);
    }

    int size(){
        int ret = 0;
        pthread_mutex_lock(&mutex);
        ret = datas.size();
        pthread_mutex_unlock(&mutex);
        return ret;
    }

private:
    queue<T> datas;
    //初始化同步锁
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int work;

    ReleaseCallback releaseCallback;
};