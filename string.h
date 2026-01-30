#include <algorithm>
#include <cstring>
#include <iostream>

class String {
 private:
  size_t sz = 0;
  size_t cap = 0;
  char* string = nullptr;

  void NewCapacity(int new_capacity) {
    char* new_array = new char[new_capacity];
    memcpy(new_array, string, sz + 1);
    delete[] string;
    string = new_array;
  }

 public:

  bool isequal(const String& other) const {
    if (sz != other.sz) {
      return false;
    }
    return (std::memcmp(string, other.string, sz) == 0);
  }

  bool islower(const String& other) const {
    return (std::memcmp(string, other.string, std::min(sz, other.sz)) < 0);
  }

  String(const char* c_string) {
    for (;c_string[sz] != '\0'; ++sz) {}
    cap = sz + 1;
    string = new char[cap];
    memcpy(string, c_string, sz + 1);
  }

  String(size_t n, char c)
          : sz(n)
          , cap(n + 1)
          , string(new char[n + 1])
  {
    memset(string, c, n);
    string[n] = '\0';
  }

  String()
          : sz(0)
          , cap(1)
          , string(new char[1])
  {
    string[0] = '\0';
  }

  String(const String& other_string)
          : sz(other_string.sz)
          , cap(other_string.cap)
          , string(new char[other_string.cap])
  {
    memcpy(string, other_string.string, sz + 1);
  }

  String& operator=(const String& other_string) {
    if (&other_string == this) return *this;
    if (sz < other_string.sz) {
      cap = other_string.cap;
      delete[] string;
      string = new char[cap];
    }
    memcpy(string, other_string.string, other_string.sz + 1);
    sz = other_string.sz;
    return *this;
  }

  char& operator[](size_t index) {
    return string[index];
  }

  const char& operator[](size_t index) const {
    return string[index];
  }

  size_t length() const {
    return sz;
  }

  size_t size() const {
    return sz;
  }

  size_t capacity() const{
    return cap - 1;
  }

  void push_back(char element) {
    size_t new_size = sz + 1;
    if (new_size + 1 >= cap) {
      cap = new_size * 2 + 1;
      NewCapacity(cap);
    }
    string[sz] = element;
    string[sz + 1] = '\0';
    sz = new_size;
  }

  void pop_back() {
    sz -= 1;
    string[sz] = '\0';
  }

  char& front() {
    return string[0];
  }

  const char& front() const {
    return string[0];
  }

  char& back() {
    return string[sz - 1];
  }

  const char& back() const {
    return string[sz - 1];
  }

  String& operator+=(char element) {
    push_back(element);
    return *this;
  }

  String& operator+=(const String& other_string) {
    size_t new_size = sz;
    new_size += other_string.sz;
    if (new_size + 1 >= cap) {
      cap = 2 * new_size + 1;
      NewCapacity(cap);
    }
    memcpy(string + sz, other_string.string, other_string.sz + 1);
    sz = new_size;
    return *this;
  }

  size_t find(const String& substring) const {
    for (int i = 0; i <= (static_cast<int>(sz) - static_cast<int>(substring.sz)); ++i) {
      if (memcmp(string + i, substring.string, substring.sz) == 0) {
        return i;
      }
    }
    return sz;
  }

  size_t rfind(const String& substring) const {
    for (int i = (static_cast<int>(sz) - static_cast<int>(substring.sz)); i >= 0; --i) {
      if (memcmp(string + i, substring.string, substring.sz) == 0) {
        return i;
      }
    }
    return sz;
  }

  String substr(size_t start, size_t count) const {
    String answer(count, '.');
    memcpy(answer.string, string + start, count);
    return answer;
  }

  bool empty() const{
    return sz == 0;
  }

  void clear(){
    sz = 0;
    string[sz] = '\0';
  }

  void shrink_to_fit() {
    cap = sz + 1;
    NewCapacity(cap);
  }

  char* data() const {
    return string;
  }

  ~String() {
    delete[] string;
  }

};

bool operator==(const String& string1, const String& string2) {
  return string1.isequal(string2);
}

bool operator!=(const String& string1, const String& string2) {
  return !(string1 == string2);
}

bool operator<(const String& string1, const String& string2) {
  return string1.islower(string2);
}

bool operator<=(const String& string1, const String& string2) {
  return !(string2 < string1);
}

bool operator>(const String& string1, const String& string2) {
  return !(string1 <= string2);
}

bool operator>=(const String& string1, const String& string2) {
  return !(string1 < string2);
}

String operator+(char element, const String& string) {
  String answer(1, element);
  answer += string;
  return answer;
}

String operator+(const String& string, char element) {
  String answer = string;
  answer += element;
  return answer;
}

String operator+(const String& string1, const String string2) {
  String answer = string1;
  answer += string2;
  return answer;
}

std::ostream& operator<<(std::ostream& out, const String& string) {
  return out << string.data();
}

std::istream& operator>>(std::istream& in, String& string) {
  char element;
  while (in.get(element)) {
    if (element == ' ' || element == '\n') {
      break;
    } else {
      string.push_back(element);
    }
  }
  return in;
}

