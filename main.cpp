#include <iostream>
#include <iterator>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <boost/filesystem.hpp>
#include <cstdio>

using namespace std;

uint RUN_SIZE = 100;
vector<string> temp_files;
vector<string> runs_files;

bool heap_cmp(const istream_iterator<string> &a, const istream_iterator<string> &b) {
    return stoi(*a) > stoi(*b);
}

template <typename T> class Heap {
public:
    void push(T val) {
        x_.push_back(val);
        push_heap(x_.begin(), x_.end(), heap_cmp);
    }

    T pop(void) {
        pop_heap(x_.begin(), x_.end(), heap_cmp);
        T ret = x_.back();
        x_.pop_back();
        return(ret);
    }

    size_t size() {
        return(x_.size());
    }
private:
    vector<T> x_;
};

bool run_cmp(const string &a, const string &b) {
    return stoi(a) < stoi(b);
}

void clean_up_files() {
    for (string const& f : temp_files) {
        remove(f.c_str());
    }
}

string new_tmpfile() {
    auto temp_file = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
    cout << temp_file << "\n";
    temp_files.push_back(temp_file.native());

    return(temp_file.native());
}

void split_runs(ifstream &input) {

    string read_buf;
    vector<string> sort_buffer;
    sort_buffer.reserve(RUN_SIZE);
    uint counter = 0;

    while (getline(input, read_buf)) {
        if (sort_buffer.size() >= RUN_SIZE) {
            string out_path = new_tmpfile();
            ofstream out_file(out_path);
            temp_files.push_back(out_path);

            sort(sort_buffer.begin(), sort_buffer.end(), run_cmp);
            for (auto const& line : sort_buffer) {
                out_file << line << "\n";
            }

            runs_files.push_back(out_path);
            sort_buffer.clear();
        }

        sort_buffer.push_back(read_buf);
        counter++;
    }

    if (sort_buffer.size() > 0) {
        string out_path = new_tmpfile();
        ofstream out_file(out_path);
        temp_files.push_back(out_path);

        sort(sort_buffer.begin(), sort_buffer.end(), run_cmp);
        for (auto const& line : sort_buffer) {
            out_file << line << "\n";
        }

        runs_files.push_back(out_path);
        sort_buffer.clear();
    }
}

void merge_runs() {
    if (runs_files.size() == 1) {
        return;
    }

    Heap<istream_iterator<string> > heap;
    vector<unique_ptr<ifstream> > in_streams;

    while (runs_files.size() > 0) {
        string f = runs_files.back();
        runs_files.pop_back();

        in_streams.push_back(make_unique<ifstream>(f));
        heap.push(
            istream_iterator<string>(*in_streams.back())
        );
    }

    while (heap.size() > 0) {
        auto x = heap.pop();
        cout << *x << "\n";

        auto next = ++x;
        if (next != istream_iterator<string>()) {
            heap.push(next);
        }
    }

    // merge_runs();
}

int main(int argc, char* argv[]) {
    atexit(clean_up_files);

    int length = 8192;
    char buffer[length];
    ifstream input;
    input.rdbuf()->pubsetbuf(buffer, length);
    input.open("file.txt");

    split_runs(input);
    merge_runs();

    // char fget_buf[1024];
    // while (fgets(fget_buf, 1023, open_files.back())) {
    //     cout << fget_buf;
    // }
    // ifstream f1("file1.txt");
    // ifstream f2("file2.txt");
    // ifstream f3("file3.txt");

    // istream_iterator<string> is_it1(f1);
    // istream_iterator<string> is_it2(f2);
    // istream_iterator<string> is_it3(f3);

    // vector<ValFilePair> heap;

    // heap.push_back(ValFilePair {*is_it1, is_it1++});
    // heap.push_back(ValFilePair {*is_it2, is_it2++});
    // heap.push_back(ValFilePair {*is_it3, is_it3++});

    // make_heap(heap.begin(), heap.end(), cmp);

    // while (heap.size() > 0) {
    //     pop_heap(heap.begin(), heap.end(), cmp);
    //     auto x = heap.back();
    //     heap.pop_back();
    //     cout << x.val << "\n";

    //     auto next = ++x.file_it;
    //     if (next != istream_iterator<string>()) {
    //         heap.push_back(ValFilePair {*next, next});
    //     }
    //     push_heap(heap.begin(), heap.end(), cmp);
    // }
    
    // sort_heap(heap.begin(), heap.end(), cmp);

    // for (auto const x : heap) {
    //     cout << x.val << "\n";
    // }
}
