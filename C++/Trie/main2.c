
#include <string.h>
#include <time.h>
#include <assert.h>

#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
using namespace std;

class Trie {
public:
    void insert(const string &str);
    set<string>& get(const string& str) const;
    Trie();
    ~Trie();
    int getNCount() const { return m_nCount; }
    int getSCount() const { return m_sCount; }
private:
    struct Node {
        vector<pair<unsigned char, Node*> > childs;
        set<string> strs;
        Node() { childs.reserve(4); }
        Node* getOrCreateChild(unsigned char c) {
            if (Node *p = getChild(c)) return p;
            childs.push_back(make_pair(c, new Node()));
            return childs.back().second;
        }
        Node* getChild(unsigned char c) {
            for (int i = 0; i < (int)childs.size(); ++i) {
                if (childs[i].first == c) return childs[i].second;
            }
            return NULL;
        }
        void destroy() {
            for (int i = 0; i < (int)childs.size(); ++i) childs[i].second->destroy();
            delete this;
        }
    };
private:
    void _insert(Node *node, const char *s, int d, int pos, const string& str);
private:
    Node *m_root;
    int m_nCount;
    int m_sCount;
};

void Trie::insert(const string &str) {
    ++m_sCount;
    for (int i = 0; i < (int)str.size(); ++i) {
        _insert(m_root, str.c_str() + i, 0, i, str);
    }
}
set<string>& Trie::get(const string& str) const {
    static set<string> EMPTY_SET;

    Node *n = m_root;
    for (int i = 0; i < (int)str.size(); ++i) {
        n = n->getChild(str[i]);
        if (n == NULL) return EMPTY_SET;
    }
    return n->strs;
}
Trie::Trie(): m_root(new Node), m_nCount(1), m_sCount(0) {
}
Trie::~Trie() {
    m_root->destroy();
}
void Trie::_insert(Node* node, const char *s, int d, int pos, const string& str) {
    if (d > 0) {
        // str, pos, pos + d
        node->strs.insert(str);
    }
    if (s[0] != 0) {
        _insert(node->getOrCreateChild(s[0]), s + 1, d + 1, pos, str);
    }
}

int main() {
    Trie t;

    getchar();

    {
        clock_t start = clock();
        ifstream fi("1.txt");
        for (string w; fi >> w; ) {
            int i = 0;
            for (; i < (int)w.size() && !isalnum(w[i]); ++i);
            int j = i;
            for (; j < (int)w.size() && isalnum(w[j]); ++j);
            t.insert(w.substr(i, j - i));
        }
        printf("read %d words, cose %fs, %d nodes\n", t.getSCount(), float(clock() - start) / CLOCKS_PER_SEC, t.getNCount());
    }

    for (string line; getline(cin, line); ) {
        set<string> &res = t.get(line);
        copy(res.begin(), res.end(), ostream_iterator<string>(cout, ","));
        puts("");
    }

    return 0;
}