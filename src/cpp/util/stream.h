#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_STREAM_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_STREAM_H_

#include <streambuf>
#include <ostream>
#include <iostream>
#include <vector>

namespace bitcoinjs {

template <class cT, class traits> class BasicCountStream;

template <class cT, class traits = std::char_traits<cT> >
class BasicCountBuffer : public std::basic_streambuf<cT, traits>
{
 public:
  BasicCountBuffer() : count(0)
  {
  }

 private:
  typename traits::int_type overflow(typename traits::int_type c)
  {
    count++;
    return traits::not_eof(c); // indicate success
  }

  std::streamsize xsputn(const cT *s, std::streamsize n)
  {
    count += n;
    return n;
  }

  size_t count;

  friend class BasicCountStream<cT, traits>;
};

template <class cT, class traits = std::char_traits<cT> >
class BasicCountStream : public std::basic_ostream<cT, traits>
{
 public:
  BasicCountStream():
  std::basic_ios<cT, traits>(&m_sbuf),
  std::basic_ostream<cT, traits>(&m_sbuf)
  {
    init(&m_sbuf);
  }

  size_t size()
  {
    return m_sbuf.count;
  }

 private:
  BasicCountBuffer<cT, traits> m_sbuf;
};

typedef BasicCountStream<char> CountStream;

template <class cT, class traits> class BasicVectorStream;

/**
 * Simple vector-backed, unconnected stream buffer.
 *
 * This buffer provides an easy way to make a vector readable by
 * stream readers and writable by stream writers, however not both. To
 * keep this class simple for now, it does not connect its input and
 * output streams.
 */
template <class cT, class traits = std::char_traits<cT> >
class BasicVectorBuffer : public std::basic_streambuf<cT, traits>
{
  typedef std::vector<cT> vector_t;

 public:
  BasicVectorBuffer() {}
  BasicVectorBuffer(const vector_t &v) : buf(v)
  {
    UpdatePointers();
  }

 private:
  void UpdatePointers()
  {
    setg(buf.data(), buf.data(), buf.data() + buf.size());
  }

  typename traits::int_type overflow(typename traits::int_type c)
  {
    buf.push_back(c);
    return traits::not_eof(c);
  }

  vector_t buf;

  friend class BasicVectorStream<cT, traits>;
};

template <class cT, class traits = std::char_traits<cT> >
class BasicVectorStream : public std::basic_iostream<cT, traits>
{
 public:
  BasicVectorStream():
  std::basic_ios<cT, traits>(&m_sbuf),
  std::basic_iostream<cT, traits>(&m_sbuf)
  {
    init(&m_sbuf);
  }

 BasicVectorStream(const std::vector<cT> &vec):
  std::basic_ios<cT, traits>(&m_sbuf),
  std::basic_iostream<cT, traits>(&m_sbuf),
  m_sbuf(vec)
  {
    init(&m_sbuf);
  }

  cT* Data() const
  {
    return m_sbuf.gptr();
  }

  size_t Size() const
  {
    return m_sbuf.egptr()-m_sbuf.gptr();
  }

  size_t Pos() const
  {
    return m_sbuf.buf.size();
  }

  std::string Str()
  {
    return std::string(m_sbuf.buf.begin(), m_sbuf.buf.end());
  }

  std::vector<cT> Vec()
  {
    return m_sbuf.buf;
  }

  std::vector<cT>& VecRef()
  {
    return m_sbuf.buf;
  }

  void SwapVec(std::vector<cT> &vec)
  {
    m_sbuf.buf.swap(vec);
    m_sbuf.UpdatePointers();
  }

 private:
  BasicVectorBuffer<cT, traits> m_sbuf;
};

typedef BasicVectorStream<char> VectorStream;

template <class cT, class traits> class BasicMemStream;

template <class cT, class traits = std::char_traits<cT> >
class BasicMemBuffer : public std::basic_streambuf<cT, traits>
{
public:
  BasicMemBuffer(cT *data, size_t len) :
    data_(data), len_(len), pos_(0) {}

private:
  typename traits::int_type overflow(typename traits::int_type c)
  {
    if (pos_ < len_) {
      data_[pos_++] = c;
      return traits::not_eof(c);
    } else {
      return traits::eof();
    }
  }

  cT *data_;
  size_t len_;
  size_t pos_;

  friend class BasicMemStream<cT, traits>;
};

template <class cT, class traits = std::char_traits<cT> >
class BasicMemStream : public std::basic_iostream<cT, traits>
{
 public:
  BasicMemStream(cT *data, size_t len) :
    std::basic_ios<cT, traits>(&sbuf_),
    std::basic_iostream<cT, traits>(&sbuf_),
    sbuf_(data, len)
  {
    init(&sbuf_);
  }

  cT* Data() const
  {
    return sbuf_.gptr();
  }

  size_t Size() const
  {
    return sbuf_.len_;
  }

  size_t Pos() const
  {
    return sbuf_.pos_;
  }

  std::string Str()
  {
    return std::string(sbuf_.data_, sbuf_.len_);
  }

 private:
  BasicMemBuffer<cT, traits> sbuf_;
};

typedef BasicMemStream<char> MemStream;

} // bitcoinjs

#endif
