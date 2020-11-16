
#include <stdio.h>
#include <stdlib.h>

#include <cstring>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <queue>
using namespace std;

#define MAX_ALPHA 256
#define BYTE_SIZE 8
#define INT32_SIZE 32
#define BYTE_MASK (1 << (BYTE_SIZE - 1))
#define INT32_MASK (1 << (INT32_SIZE - 1))
#define _BIT_(N, I) ((((N) & (1 << (I))) >> (I)))
#define BITS_COUNT(N, c)                      \
    do {                                      \
        unsigned char t = (N);                \
        for (int i = 0; i < BYTE_SIZE; i++) { \
            if (((t) >>= 1)) (c)++;           \
        }                                     \
    } while (0);

typedef unsigned char byte_8;

#define DEBUG 0

class BinaryStd {
   private:
    byte_8 __byteBits_w;
    byte_8 __byteBits_r;

    FILE* _in;
    FILE* _out;

    int __bit_read_ptr;
    int __bit_write_ptr;

    bool __not_enough_byte;
    int __index_count;
    bool __bit;

   public:
    BinaryStd() {
        __bit = __not_enough_byte = false;
        __byteBits_w = __bit_write_ptr = __bit_read_ptr = __not_enough_byte =
            __byteBits_w = 0;
        _in = _out = NULL;
    }
    BinaryStd(const char* in_file, const char* out_file) : BinaryStd() {
        _in = fopen(in_file, "rb+");
        _out = fopen(out_file, "wb+");
    }
    ~BinaryStd() { close(); }

    void rewind_file_ptr() { rewind(_in); }
    bool eof() { return feof(_in); }
    void close() {
        if (_in) {
            fclose(_in);
            _in = NULL;
        }
        if (_out) {
            fclose(_out);
            _out = NULL;
        }
    }
    void writeBit(bool bit) {
        // 保存位
        __byteBits_w <<= 1;
        if (bit) __byteBits_w |= 1;
        __bit_write_ptr++;
        if (__bit_write_ptr >= BYTE_SIZE) {
            // 写入1字节作为缓存空间
            fwrite((void*)&__byteBits_w, 1, 1, _out);
            // 每次写入一块1字节的bit串，就重置bit流指针
            __byteBits_w = 0;
            __bit_write_ptr = 0;
        }
        fflush(_out);
    }

    void writeEndBits() {
        if (__byteBits_w && __bit_write_ptr < BYTE_SIZE) {
            // 不足8bit的后面补零
            __byteBits_w <<= (BYTE_SIZE - __bit_write_ptr);
            // 必须重置0，重新写入一个8bits流
            __bit_write_ptr = 0;
            // 写入8bits
            writeByte(__byteBits_w);
            __byteBits_w = 0;
        }
        fflush(_out);
    }
    /* 写入8bits */
    void writeByte(byte_8 byte) {
        int k = 0;
        while (k < BYTE_SIZE) {
            writeBit(_BIT_(byte, BYTE_SIZE - 1 - k));
            k++;
        }
    }
    /* 由于字节序问题，不能直接fwrite()写入4字节int */
    void writeInt(int integer) {
        int k = 0;
        while (k < INT32_SIZE) {
            // get 1 byte
            byte_8 by = (integer >> (INT32_SIZE - BYTE_SIZE));
            writeByte(by);
            integer <<= BYTE_SIZE;
            k += BYTE_SIZE;
        }
    }
    /* 读取1bit */
    bool readBit() {
        // 不足8bits，再次从缓存空间 [__byteBits_w] 中读取余下的对应位
        if (__not_enough_byte) {
            // __index_count<0 表示读取完成
            if (__index_count < 0) return false;
            // 从__byteBits_w取指定的__index_count位
            __bit = _BIT_(__byteBits_w, __index_count);
            __index_count--;
            return __bit;
        }
        // 8的倍数
        if (__bit_read_ptr >= BYTE_SIZE || __bit_read_ptr <= 0) {
            fflush(_in);
            // 当__bit_read_ptr达到8bits时，再次从文件读取1字节
            size_t r = fread((void*)&__byteBits_r, 1, 1, _in);
            // 第一次发现不足8位,fread()返回值<0说明读取失败
            if (r <= 0) {
                __not_enough_byte = true;
                // 不足位有__index_count位
                BITS_COUNT(__byteBits_w, __index_count);
                __bit = _BIT_(__byteBits_w, __index_count);
                __index_count--;
                return __bit;
            }
            __bit_read_ptr = 0;
        }
        __bit = _BIT_(__byteBits_r, (BYTE_SIZE - 1 - __bit_read_ptr));
        __bit_read_ptr++;
        return __bit;
    }
    // 从二进制文件读取字符char，判断结束标准是fread()返回0
    size_t getChar(char& c) { return fread(&c, 1, 1, _in); }
    /* 读取1byte=unsigned char */
    byte_8 readByte() {
        int k = 0;
        byte_8 r = 0;
        while (k < BYTE_SIZE) {
            r = (r << 1) | (readBit());
            ++k;
        }
        return r;
    }
    /* 读取4byte也就是一个Int */
    int readInt() {
        int k = 0, r = 0;
        while (k < sizeof(int)) {
            r = (r << (BYTE_SIZE) | readByte());
            ++k;
        }
        return r;
    }
    void flush() {
        fflush(_in);
        fflush(_out);
    }
};

template <class T>
struct TreeNode {
    T data;
    int weight;
    TreeNode* parent;
    TreeNode* left;
    TreeNode* right;
    TreeNode(T data, int weight, TreeNode* left = nullptr,
             TreeNode* right = nullptr) {
        this->data = data;
        this->weight = weight;
        this->left = left;
        this->right = right;
    }
    bool isLeaf() { return this->left == nullptr && this->right == nullptr; }
};

template <class T>
class HuffmanTree {
    struct Compare {
        bool operator()(TreeNode<T>* t1, TreeNode<T>* t2) {
            return t1->weight > t2->weight;
        }
    };

   private:
    TreeNode<T>* m_root;
    unordered_map<T, string> m_table;
    // 字符-频率表
    unordered_map<T, byte_8> m_table_freq;
    BinaryStd* bs;

   public:
    HuffmanTree(string in_filename, string out_filename) {
        m_root = nullptr;
        bs = new BinaryStd(in_filename.c_str(), out_filename.c_str());
    }
    ~HuffmanTree() {
        clear(m_root);
        delete bs;
        bs = nullptr;
    }

   protected:
    void flush_and_close() {
        this->bs->flush();
        this->bs->close();
    }

    void clear(TreeNode<T>*& root) {
        if (!root) return;
        clear(root->left);
        clear(root->right);
        delete root;
        root = nullptr;
    }
    // 构造哈夫曼树
    void buildTree() {
        priority_queue<TreeNode<T>*, vector<TreeNode<T>*>, Compare> pq;
        for (auto i = m_table_freq.begin(); i != m_table_freq.end(); ++i) {
            TreeNode<T>* node = new TreeNode<T>(i->first, i->second);
            pq.push(node);
        }
        if (pq.empty()) throw exception();
        while (pq.size() != 1) {
            auto t1 = pq.top();  // left
            pq.pop();
            auto t2 = pq.top();  // right
            pq.pop();

            TreeNode<T>* node = new TreeNode<T>(0, t1->weight + t2->weight);
            node->left = t1;
            node->right = t2;
            pq.push(node);
        }
        // 剩下最后一个最大元素
        this->m_root = pq.top();
        pq.pop();
    }
    // 计算字符频率表
    string calc_char_frequency() {
        string origin_text = "";
        char c;
        while ((bs->getChar(c))) {
            origin_text += c;
            m_table_freq[c]++;
        }
        bs->rewind_file_ptr();
        return origin_text;
    }
    // 构造编码表
    void buildCode(TreeNode<T>* root, string s) {
        // 叶子节点
        if (root->left == nullptr && root->right == nullptr) {
// 输出编码表
#if DEBUG
            cout << root->data << ": " << s << endl;
#endif
            m_table[root->data] = s;
            return;
        }
        buildCode(root->left, s + "0");
        buildCode(root->right, s + "1");
    }

    string to_binary(char ch) {
        string s = "";
        unsigned mask = 1 << 7;
        for (int i = 0; i < 8; i++) {
            if (ch & mask)
                s += '1';
            else
                s += '0';
            ch <<= 1;
        }
        return s;
    }

    /*
    '0'表示内部节点，
    '1'表示叶子节点，
    若每次读取到1时，接下来8比特则会被解读是叶子节点（char）
    */
    void writeHuffmanTree(TreeNode<T>* node) {
        // 叶子节点
        if (node->left == nullptr && node->right == nullptr) {
            bs->writeBit(true);
#if DEBUG
            cout << true << "." << to_binary(node->data) << ".";
#endif
            bs->writeByte(node->data);
            return;
        }
#if DEBUG
        cout << false;
#endif
        bs->writeBit(false);
        writeHuffmanTree(node->left);
        writeHuffmanTree(node->right);
    }

    TreeNode<T>* readHuffmanTree() {
        // 叶子节点
        if (bs->readBit()) {
            // 读取叶子节点中的字符
            char c = (char)bs->readByte();
            return new TreeNode<T>(c, 0);
        }
        // 非叶子节点，继续构造哈夫曼树
        TreeNode<T>* left = readHuffmanTree();
        TreeNode<T>* right = readHuffmanTree();
        return new TreeNode<T>('\0', 0, left, right);
    }

   public:
    // 开始编码
    void encode() {
        // 统计出现次数
        string origin_text = calc_char_frequency();
        // 构造哈夫曼编码树
        buildTree();
        // 构造编码表
        buildCode(m_root, "");
        // 先写入哈夫曼编码树，用于解码
        writeHuffmanTree(m_root);
        // 再写入字符数，用于解码
        int size = origin_text.size();
        bs->writeInt(size);
        // 最后使用编码表编码成bits流，并写入文件
        for (int i = 0; i < size; i++) {
            // 对应字符的01编码
            string bits = m_table[origin_text[i]];
            for (int j = 0; j < bits.size(); j++) {
                if (bits[j] == '1')
                    bs->writeBit(true);
                else
                    bs->writeBit(false);
            }
        }
        // 结尾处理
        bs->writeEndBits();
        bs->flush();
        flush_and_close();
    }
    /*
    为了找到与当前位相对应的字符，我们使用以下简单步骤。
    1.我们从根开始，然后进行跟踪直到找到叶子。
    2.如果当前位为0，则移至树的左侧节点。
    3.如果该位为1，则移至树的右节点。
    4.如果在遍历期间遇到一个叶子节点，则打印该特定叶子节点的字符，然后再次从步骤1开始继续进行编码数据的迭代。
     */
    void decode() {
        // 重构哈夫曼树
        TreeNode<T>* root = readHuffmanTree();
        // 编码前的字符长度 32bits
        int N = bs->readInt();
        // 从哈夫曼树中找出每个编码对应的字符
        for (int i = 0; i < N; i++) {
            // 从头开始
            TreeNode<T>* cur = root;
            // 非叶子节点
            while (!cur->isLeaf()) {
                bool bit = bs->readBit();
                if (bit)
                    cur = cur->right;
                else
                    cur = cur->left;
            }
            // 叶子节点，说明找到了字符
            bs->writeByte(cur->data);
#if DEBUG
            cout << "." << to_binary(cur->data) << ".";
#endif
        }
        flush_and_close();
    }
};
