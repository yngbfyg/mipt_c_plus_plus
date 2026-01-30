#include <iostream>
#include <string>
#include <vector>

class Rational;

class BigInteger {
  friend class Rational;
  friend bool operator==(const BigInteger& bigint, const BigInteger& other_bigint);
  friend bool operator<(const BigInteger& bigint, const BigInteger& other_bigint);
  friend bool operator==(const Rational& rat, const Rational& other_rat);
  friend bool operator<(const Rational& rat, const Rational& other_rat);

 private:
  static const int mod = 1e9;
  bool is_positive_ = true;
  std::vector<int> number;


  bool isnul() const{
    return (number.size() == 1 && number[0] == 0);
  }


  bool islower(const BigInteger& other_bigint) {
    if (number.size() != other_bigint.number.size()) return number.size() < other_bigint.number.size();
    for (int i = number.size() - 1; i >= 0; --i) {
      if (number[i] != other_bigint.number[i]) return number[i] < other_bigint.number[i];
    }
    return false;
  }

  void plus(const BigInteger& other_bigint) {
    int remain = 0;
    for (size_t i = 0; i < std::max(number.size(), other_bigint.number.size()); ++i) {
      if (i < other_bigint.number.size()) {
        if (i >= number.size()) {
          number.push_back(0);
        }
        long long current = number[i] + other_bigint.number[i] + remain;
        number[i] = current % mod;
        remain = current / mod;
      } else {
        long long current = number[i] + remain;
        number[i] = current % mod;
        remain = current / mod;
      }
    }
    if (remain != 0) {
      number.push_back(remain);
    }
    while (number.size() != 0 && number[number.size() - 1] == 0) number.pop_back();
    if (number.size() == 0) number.push_back(0);
    is_positive_ = true;
  }

  void minus(const BigInteger& other_bigint) {
    BigInteger max_number;
    BigInteger min_number;
    if (islower(other_bigint)) {
      min_number = *this;
      max_number = other_bigint;
    } else {
      max_number = *this;
      min_number = other_bigint;
    }
    bool istaken = false;
    int long long current = 0;
    for (size_t i = 0; i < max_number.number.size(); ++i) {
      if (i < min_number.number.size()) min_number.number.push_back(0);
      current = max_number.number[i] - (int)istaken;
      if (current < min_number.number[i]) {
        current += mod;
        istaken = true;
      } else istaken = false;
      current -= min_number.number[i];
      max_number.number[i] = current;
    }
    while (max_number.number.size() != 0 && max_number.number[max_number.number.size() - 1] == 0) max_number.number.pop_back();
    if (max_number.number.size() == 0) max_number.number.push_back(0);
    *this = max_number;
    is_positive_ = true;
  }

  BigInteger my_pow(int num) {
    if (num == 0) return 1;
    if (num == 1) return *this;
    if (num % 2 == 1) {
      BigInteger answer = my_pow(num - 1);
      answer *= *this;
      return answer;
    }
    BigInteger answer = my_pow(num / 2);
    answer *= answer;
    return answer;
  }

  long long my_size() {
    if (isnul()) return 1;
    int first = number[number.size() - 1];
    long long answer = 0;
    while (first > 0) {
      answer += 1;
      first /= 10;
    }
    answer += 9 * (number.size() - 1);
    return answer;
  }

 public:
  BigInteger() {}

  BigInteger(std::string str) {
    if (str == "0" or str == "-0") {
      number.push_back(0);
      return;
    }
    if (str[0] == '-') {
      is_positive_ = false;
    }
    int n = str.size();
    int current_index = n - 1;

    while (current_index >= (int) !is_positive_) {
      int current_number = 0;
      for (int i = 0; i < 9 && current_index >= (int) !is_positive_; ++i, --current_index) {
        current_number += (str[current_index] - '0') * std::pow(10, i);
      }
      number.push_back(current_number);
    }
    while (number.size() != 0 && number[number.size() - 1] == 0) number.pop_back();
    if (number.size() == 0) number.push_back(0);
  }

  BigInteger(long long integer) {
    if (integer >= 0) is_positive_ = true;
    else is_positive_ = false;
    integer = std::abs(integer);
    if (integer == 0) {
      number.push_back(0);
      return;
    }
    while (integer > 0) {
      number.push_back(integer % mod);
      integer /= mod;
    }
  }

  std::string toString() const{
    std::string answer = "";
    if (!is_positive_) {
      answer += '-';
    }
    std::string cur_ans;
    answer += std::to_string(number[number.size() - 1]);
    for (int i = number.size() - 2; i >= 0; --i) {
      cur_ans = std::to_string(number[i]);
      for (int j = 0; j < 9 - static_cast<int>(cur_ans.size()); ++j) {
        answer += '0';
      }
      answer += cur_ans;
    }
    return answer;
  }

  BigInteger operator-() const{
    if (isnul()) return *this;
    BigInteger new_number = *this;
    if (is_positive_) new_number.is_positive_ = false;
    else new_number.is_positive_ = true;
    return new_number;
  }


  BigInteger& operator+=(const BigInteger& other_bigint) {
    bool cur_sign = is_positive_;
    bool islow = islower(other_bigint);
    if (other_bigint.is_positive_ == is_positive_) {
      plus(other_bigint);
      is_positive_ = cur_sign;
    } else {
      minus(other_bigint);
      if (isnul()) {
        is_positive_ = true;
      } else if (cur_sign){
        if (islow) {
          is_positive_ = false;
        } else is_positive_ = true;
      } else {
        if (islow) {
          is_positive_ = true;
        } else is_positive_ = false;
      }
    }
    return *this;
  }

  BigInteger& operator-=(const BigInteger& other_bigint) {
    *this += -other_bigint;
    return *this;
  }

  BigInteger& operator*=(const BigInteger& other_bigint) {
    BigInteger answer;
    if (is_positive_ == other_bigint.is_positive_) answer.is_positive_ = true;
    else is_positive_ = answer.is_positive_ = false;
    for (size_t i = 0; i < (number.size() + other_bigint.number.size()) + 1; ++i) answer.number.push_back(0);
    long long current = 0;
    for (size_t i = 0; i < number.size(); ++i) {
      current = 0;
      for (size_t j = 0; j < other_bigint.number.size(); ++j) {
        current += answer.number[i + j];
        current += (long long) number[i] * (long long) other_bigint.number[j];
        answer.number[i + j] = (int) (current % mod);
        current /= mod;
      }
      size_t j = other_bigint.number.size();
      while (current != 0) {
        current += (long long) answer.number[i + j];
        answer.number[i + j] = (int) (current % mod);
        current /= mod;
        ++j;
      }
    }
    *this = answer;
    while (number.size() != 0 && number[number.size() - 1] == 0) number.pop_back();
    if (number.size() == 0) number.push_back(0);
    if (isnul()) is_positive_ = true;
    return *this;
  }

  BigInteger& operator/=(const BigInteger& other_bigint) {
    BigInteger answer = 0;
    bool ans_sign;
    if (is_positive_ == other_bigint.is_positive_) ans_sign = true;
    else ans_sign = false;
    BigInteger current = 0;
    if (islower(other_bigint)) {
      *this = 0;
      return *this;
    }
    BigInteger absother = other_bigint;
    absother.is_positive_ = true;
    int indexofchecked = number.size() - 1;
    for (int i = number.size() - 1; current.islower(other_bigint) && i >= 0; --i) {
      current *= mod;
      current += number[i];
      indexofchecked -= 1;
    }
    for (int i = indexofchecked; i >= 0; --i) {
      int l = 0;
      int r = mod;
      while (r - l > 1) {
        int mid = (r + l) / 2;
        BigInteger check = mid;
        check *= absother;
        if (current.islower(check)) r = mid;
        else l = mid;
      }
      BigInteger remain = absother;
      remain *= l;
      current -= remain;
      current *= mod;
      current += number[i];
      answer *= mod;
      answer += l;
    }

    int l = 0;
    int r = mod;
    while (r - l > 1) {
      int mid = (r + l) / 2;
      BigInteger check = mid;
      check *= absother;
      if (current.islower(check)) r = mid;
      else l = mid;
    }
    answer *= mod;
    answer += l;

    *this = answer;
    is_positive_ = ans_sign;
    while (number.size() != 0 && number[number.size() - 1] == 0) number.pop_back();
    if (number.size() == 0) number.push_back(0);
    if (isnul()) is_positive_ = true;
    return *this;
  }

  BigInteger& operator%=(const BigInteger& other_bigint) {
    BigInteger answer = *this;
    answer /= other_bigint;
    answer *= other_bigint;
    *this -= answer;
    while (number.size() != 0 && number[number.size() - 1] == 0) number.pop_back();
    if (number.size() == 0) number.push_back(0);
    if (isnul()) is_positive_ = true;
    return *this;
  }

  BigInteger& operator++() {
    *this += 1;
    return *this;
  }

  BigInteger operator++(int) {
    BigInteger current = *this;
    ++(*this);
    return current;
  }

  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }

  BigInteger operator--(int) {
    BigInteger current = *this;
    *this -= 1;
    return current;
  }

  explicit operator bool() const {
    if (number.size() == 1 && number[0] == 0) return 0;
    return 1;
  }

  static BigInteger gcd(BigInteger num1, BigInteger num2);
};


bool operator==(const BigInteger& bigint, const BigInteger& other_bigint) {
  if (bigint.number.size() != other_bigint.number.size()) return false;
  for (size_t i = 0; i < bigint.number.size(); ++i) {
    if (bigint.number[i] != other_bigint.number[i]) return false;
  }
  return bigint.is_positive_ == other_bigint.is_positive_;
}

bool operator!=(const BigInteger& bigint, const BigInteger& other_bigint) {
  return !(bigint == other_bigint);
}

bool operator<(const BigInteger& bigint, const BigInteger& other_bigint) {
  if (bigint.is_positive_ != other_bigint.is_positive_) {
    return bigint.is_positive_ == false;
  }
  if (bigint.is_positive_ == true) {
    if (bigint.number.size() != other_bigint.number.size()) return bigint.number.size() < other_bigint.number.size();
    for (int i = bigint.number.size() - 1; i >= 0; --i) {
      if (bigint.number[i] != other_bigint.number[i]) return bigint.number[i] < other_bigint.number[i];
    }
    return false;
  }
  if (bigint.number.size() != other_bigint.number.size()) return bigint.number.size() > other_bigint.number.size();
  for (int i = bigint.number.size() - 1; i >= 0; --i) {
    if (bigint.number[i] != other_bigint.number[i]) return bigint.number[i] > other_bigint.number[i];
  }
  return false;
}

bool operator>(const BigInteger& bigint, const BigInteger& other_bigint) {
  return other_bigint < bigint;
}

bool operator<=(const BigInteger& bigint, const BigInteger& other_bigint) {
  return !(other_bigint < bigint);
}

bool operator>=(const BigInteger& bigint, const BigInteger& other_bigint) {
  return !(bigint < other_bigint);
}

BigInteger BigInteger::gcd(BigInteger num1, BigInteger num2) {
  BigInteger cur1;
  BigInteger cur2;
  if (num1 > num2) std::swap(num1, num2);
  while (num1 > 0) {
    cur1 = num1;
    cur2 = num2;
    cur2 %= num1;
    num1 = cur2;
    num2 = cur1;
  }
  return num2;
}


BigInteger operator "" _bi(const char* s, size_t) {
  std::string str = s;
  BigInteger newBigInt = str;
  return newBigInt;
}

BigInteger operator "" _bi(const char* s) {
  std::string str = s;
  BigInteger newBigInt = str;
  return newBigInt;
}

BigInteger operator "" _bi(const unsigned long long number) {
  BigInteger newBigInt(number);
  return newBigInt;
}


BigInteger operator*(const BigInteger& num1, const BigInteger& num2) {
  BigInteger answer = num1;
  answer *= num2;
  return answer;
}

BigInteger operator+(const BigInteger& num1, const BigInteger& num2) {
  BigInteger answer = num1;
  answer += num2;
  return answer;
}

BigInteger operator-(const BigInteger& num1, const BigInteger& num2) {
  BigInteger answer = num1;
  answer -= num2;
  return answer;
}

BigInteger operator/(const BigInteger& num1, const BigInteger& num2) {
  BigInteger answer = num1;
  answer /= num2;
  return answer;
}

BigInteger operator%(const BigInteger& num1, const BigInteger& num2) {
  BigInteger answer = num1;
  answer %= num2;
  return answer;
}

std::istream& operator>>(std::istream& in, BigInteger& number) {
  std::string str;
  in >> str;
  BigInteger new_number(str);
  number = new_number;
  return in;
}

std::ostream& operator<<(std::ostream& out, const BigInteger& number) {
  return out << number.toString();
}


class Rational {
  friend bool operator==(const Rational& rat, const Rational& other_rat);
  friend bool operator<(const Rational& rat, const Rational& other_rat);

 private:

  BigInteger nominator;
  BigInteger denominator;

  void reduction() {
    bool cur_sign = nominator.is_positive_;
    nominator.is_positive_ = true;
    BigInteger del = BigInteger::gcd(nominator, denominator);
    nominator /= del;
    denominator /= del;
    nominator.is_positive_ = cur_sign;
  }

  void sign_stabilization() {
    nominator.is_positive_ = nominator.is_positive_ * denominator.is_positive_;
    denominator.is_positive_ = true;
  }

 public:
  Rational() {}

  Rational (const BigInteger& bigint) {
    nominator = bigint;
    denominator = 1;
  }

  Rational (const int integer) {
    nominator = integer;
    denominator = 1;
  }

  std::string toString() const {
    std::string answer = "";
    answer += nominator.toString();
    if (denominator > 1) {
      answer += '/' + denominator.toString();
    }
    return answer;
  }

  Rational operator-() const{
    if (nominator.isnul()) return *this;
    Rational new_number = *this;
    new_number.nominator *= -1;
    return new_number;
  }

  Rational& operator+=(const Rational& other_rat) {
    nominator *= other_rat.denominator;
    nominator += other_rat.nominator * denominator;
    denominator *= other_rat.denominator;
    sign_stabilization();
    reduction();
    if (nominator.isnul()) {
      nominator.is_positive_ = true;
      denominator = 1;
    }
    return *this;
  }

  Rational& operator-=(const Rational& other_rat) {
    (*this) += (-other_rat);
    return *this;
  }

  Rational& operator*=(const Rational& other_rat) {
    nominator *= other_rat.nominator;
    denominator *= other_rat.denominator;
    sign_stabilization();
    reduction();
    if (nominator.isnul()) {
      denominator = 1;
      nominator.is_positive_ = true;
    }
    return *this;
  }

  Rational& operator/=(const Rational& other_rat) {
    nominator *= other_rat.denominator;
    denominator *= other_rat.nominator;
    sign_stabilization();
    reduction();
    if (nominator.isnul()) {
      denominator = 1;
      nominator.is_positive_ = true;
    }
    return *this;
  }

  std::string asDecimal(size_t precision = 0) const {
    std::string answer = "";
    answer += (nominator / denominator).toString();
    if (precision == 0) return answer;
    BigInteger remain = nominator % denominator;
    if (remain < 0) {
      remain *= -1;
    }
    answer += '.';
    BigInteger ten = 10;
    remain *= ten.my_pow(precision);
    remain /= denominator;
    for (int i = 0; i < static_cast<int>(precision) - static_cast<int>(remain.my_size()); ++i) answer += '0';
    answer += remain.toString();
    if (!nominator.is_positive_ && answer[0] != '-') {
      answer = '-' + answer;
    }
    return answer;
  }

  explicit operator double() const {
    return std::stod(asDecimal(10));
  }

};

bool operator==(const Rational& rat, const Rational& other_rat) {
  return (rat.nominator == other_rat.nominator && rat.denominator == other_rat.denominator);
}

bool operator!=(const Rational& rat, const Rational& other_rat) {
  return !(rat == other_rat);
}

bool operator<(const Rational& rat, const Rational& other_rat) {
  if (rat.nominator.is_positive_ != other_rat.nominator.is_positive_) {
    return !rat.nominator.is_positive_;
  }
  return rat.nominator * other_rat.denominator < other_rat.nominator * rat.nominator;
}

bool operator>(const Rational& rat, const Rational& other_rat) {
  return other_rat < rat;
}

bool operator<=(const Rational& rat, const Rational& other_rat) {
  return !(other_rat < rat);
}

bool operator>=(const Rational& rat, const Rational& other_rat) {
  return !(rat < other_rat);
}

Rational operator+(const Rational& rat, const Rational& other_rat) {
  Rational answer = rat;
  answer += other_rat;
  return answer;
}

Rational operator-(const Rational& rat, const Rational& other_rat) {
  Rational answer = rat;
  answer -= other_rat;
  return answer;
}

Rational operator*(const Rational& rat, const Rational& other_rat) {
  Rational answer = rat;
  answer *= other_rat;
  return answer;
}

Rational operator/(const Rational& rat, const Rational& other_rat) {
  Rational answer = rat;
  answer /= other_rat;
  return answer;
}

std::ostream& operator<<(std::ostream& out, const Rational& number) {
  return out << number.toString();
}
