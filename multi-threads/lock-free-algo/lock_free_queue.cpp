#include <atomic>
#include <memory>
#include <iostream>

using namespace std;

template <typename DataType>
struct node
{
	node(DataType data): data_ptr(new DataType(data)), next(nullptr) {}
	node():data_ptr(nullptr), next(nullptr) {}
	unique_ptr<DataType> data_ptr;
	atomic<node<DataType>*> next;
};

template <typename DataType>
class lock_free_queue
{
public:
	
	// 2 assumptions
	// 1. head and tail pointing to a dummy node first
	// 2. head and end are never changed
	lock_free_queue()
	{
		node<DataType>* new_node = new node<DataType>;
		end = new node<DataType>;
		new_node->next.store(end);
		head.store(new_node);
		tail.store(new_node);
	}

	~lock_free_queue()
	{
		node<DataType>* temp;
		while (head) {
			temp = head;
			head = head.load()->next.load();
			delete temp;
		}
	}

	void Push(DataType data)
	{
		node<DataType>* new_node = new node<DataType>(data);
		new_node->next.store(end);
		node<DataType>* old_tail_next = tail.load()->next.load();
		for(;;) {
			if (tail.load()->next.compare_exchange_weak(old_tail_next, new_node)) {
				// TBD this will cause false update; ABA problem
				tail.store(new_node);
				break;
			}
			else {
				old_tail_next = tail.load()->next.load();
			}
		}
	}

	shared_ptr<DataType> Pop()
	{
		if (head.load()->next.load() == end) {
			return shared_ptr<DataType>(nullptr);
		}

		node<DataType>* old_head = head.load();
		node<DataType>* head_next = old_head->next.load();
		node<DataType>* head_next_next = head_next->next.load();
		while (!head.load()->next.compare_exchange_weak(head_next, head_next_next)) {
			head_next_next = head_next->next.load();
		}
		shared_ptr<DataType> data = move(head_next->data_ptr);

		// TBD free old_head;
		return data;
	}

private:
	atomic<node<DataType>*> head;
	atomic<node<DataType>*> tail;
	node<DataType>* end;
};

int main()
{
	lock_free_queue<int> a;
	a.Push(5);
	a.Push(7);
	a.Push(9);

	cout << "Popping" << endl;
	cout << "Ans1: " << (*a.Pop()) << endl;
	cout << "Ans2: " << (*a.Pop()) << endl;
	return 0;
}

