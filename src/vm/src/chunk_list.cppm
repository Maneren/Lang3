module;

#include <forward_list>
#include <limits>
#include <new>

export module l3.vm:chunk_list;

export namespace l3::vm {

template <typename T, std::size_t ChunkSize> class ChunkedAllocator {
  class Chunk {
    union Inner {
      T value;
      Inner *next;

      Inner() : next(nullptr) {}
      Inner(const Inner &) = delete;
      Inner(Inner &&) = delete;
      Inner &operator=(const Inner &) = delete;
      Inner &operator=(Inner &&) = delete;
      ~Inner() {}
    };
    std::array<Inner, ChunkSize> data;
    std::size_t used = 0;
    std::size_t alive = 0;
    Inner *free_list = nullptr;

  public:
    Chunk() {
      for (std::size_t i = 0; i < ChunkSize - 1; ++i) {
        data[i].next = &data[i + 1];
      }
      data[ChunkSize - 1].next = nullptr;
    }
    T *allocate() {
      if (free_list) {
        Inner *slot = free_list;
        free_list = free_list->next;
        ++alive;
        return &slot->value;
      }

      if (used >= ChunkSize) {
        return nullptr;
      }

      T *ptr = &data[used].value;
      ++used;
      ++alive;
      return ptr;
    }

    bool deallocate(T *ptr) {
      if (!contains(ptr)) {
        return false;
      }

      auto *slot = reinterpret_cast<Inner *>(ptr);
      slot->next = free_list;
      free_list = slot;
      --alive;
      return true;
    }

    bool contains(const T *ptr) const {
      return ptr >= &data.cbegin()->value && ptr < &data.cend()->value;
    }

    [[nodiscard]] bool is_empty() const { return alive == 0; }

    [[nodiscard]] bool is_full() const {
      return used >= ChunkSize && free_list == nullptr;
    }

    DEFINE_VALUE_ACCESSOR_X(alive);
    DEFINE_VALUE_ACCESSOR_X(used);
  };

  mutable std::vector<std::unique_ptr<Chunk>> chunks_;
  mutable std::vector<Chunk *> free_chunks_;
  mutable Chunk *current_chunk_ = nullptr;

  Chunk *get_available_chunk() const {
    if (!free_chunks_.empty()) {
      auto chunk = free_chunks_.back();
      if (chunk->get_alive() + 1 >= ChunkSize) {
        free_chunks_.pop_back();
      }
      return chunk;
    }

    chunks_.emplace_back(std::make_unique<Chunk>());
    current_chunk_ = chunks_.back().get();
    free_chunks_.push_back(current_chunk_);
    return current_chunk_;
  }

  void cleanup_empty_chunks() const {
    if (chunks_.size() <= 1) {
      return;
    }

    for (std::size_t i = 0; i < free_chunks_.size() - 1; ++i) {
      if (free_chunks_[i]->is_empty()) {
        free_chunks_[i] = free_chunks_[free_chunks_.size() - 1];
        free_chunks_.pop_back();
      }
    }

    for (std::size_t i = 0; i < chunks_.size() - 1; ++i) {
      if (chunks_[i]->is_empty()) {
        chunks_[i] = std::move(chunks_[chunks_.size() - 1]);
        chunks_.pop_back();
      }
    }
  }

public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;

  template <typename U> struct rebind {
    using other = ChunkedAllocator<U, ChunkSize>;
  };

  ChunkedAllocator() = default;

  template <typename U>
  ChunkedAllocator(const ChunkedAllocator<U, ChunkSize> & /*unused*/) noexcept {
  }

  ChunkedAllocator(const ChunkedAllocator &) = delete;
  ChunkedAllocator &operator=(const ChunkedAllocator &) = delete;

  ChunkedAllocator(ChunkedAllocator &&) noexcept = default;
  ChunkedAllocator &operator=(ChunkedAllocator &&) noexcept = default;

  ~ChunkedAllocator() = default;

  T *allocate(std::size_t n) {
    if (n != 1) [[unlikely]] {
      return static_cast<T *>(::operator new(n * sizeof(T)));
    }

    Chunk *chunk = get_available_chunk();
    T *ptr = chunk->allocate();

    if (ptr == nullptr) {
      *(volatile char *)nullptr = 0;
    }

    return ptr;
  }

  void deallocate(T *ptr, std::size_t n) noexcept {
    if (n != 1) [[unlikely]] {
      ::operator delete(ptr);
      return;
    }

    for (auto &chunk : std::views::reverse(chunks_)) {
      if (chunk->deallocate(ptr)) {

        static thread_local size_t cleanup_counter = 0;
        if (++cleanup_counter >= ChunkSize) [[unlikely]] {
          cleanup_counter = 0;
          cleanup_empty_chunks();
        } else if (chunk->get_alive() == ChunkSize - 1) {
          free_chunks_.push_back(chunk.get());
        }
        return;
      }
    }

    // This should never happen unless there is a bug in the user code
    std::unreachable();
  }

  template <typename U, typename... Args>
  void construct(U *ptr, Args &&...args) {
    new (ptr) U(std::forward<Args>(args)...);
  }

  template <typename U> void destroy(U *ptr) { ptr->~U(); }

  std::size_t max_size() const noexcept {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  void cleanup() const { cleanup_empty_chunks(); }

  template <typename U, std::size_t OtherChunkSize>
  bool operator==(const ChunkedAllocator<U, OtherChunkSize>
                      & /*unused*/) const noexcept {
    return false;
  }
};

template <typename T, std::size_t ChunkSize = 64>
using ChunkedForwardList = std::forward_list<T, ChunkedAllocator<T, ChunkSize>>;

} // namespace l3::vm
