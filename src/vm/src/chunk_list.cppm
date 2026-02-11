export module l3.vm:chunk_list;

import std;

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
    Inner *free_list;

  public:
    Chunk() : free_list(data.data()) {
      for (std::size_t i = 0; i < ChunkSize - 1; ++i) {
        data[i].next = &data[i + 1];
      }
      data[ChunkSize - 1].next = nullptr;
    }
    T *allocate() {
      if (is_full()) {
        return nullptr;
      }

      Inner *slot = free_list;
      free_list = free_list->next;
      ++used;
      return &slot->value;
    }

    void deallocate(T *ptr) {
      auto *slot = reinterpret_cast<Inner *>(ptr);
      slot->next = free_list;
      free_list = slot;
      --used;
    }

    bool contains(const T *ptr) const {
      return ptr >= &data.cbegin()->value && ptr < &data.cend()->value;
    }

    [[nodiscard]] bool is_empty() const { return used == 0; }
    [[nodiscard]] bool is_full() const { return free_list == nullptr; }

    DEFINE_VALUE_ACCESSOR_X(used);
  };

  mutable std::vector<std::unique_ptr<Chunk>> chunks_;
  mutable std::vector<Chunk *> free_chunks_;
  mutable Chunk *current_chunk_ = nullptr;

  Chunk *get_available_chunk() const {
    if (!free_chunks_.empty()) {
      auto *chunk = free_chunks_.back();
      if (chunk->get_used() + 1 >= ChunkSize) {
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

    for (auto &chunk_ptr : std::views::reverse(free_chunks_)) {
      if (chunk_ptr->is_empty()) {
        chunk_ptr = free_chunks_.back();
        free_chunks_.pop_back();
      }
    }

    for (auto &chunk_ptr : std::views::reverse(chunks_)) {
      if (chunk_ptr->is_empty()) {
        chunk_ptr = std::move(chunks_.back());
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
    if (n != 1) {
      return static_cast<T *>(::operator new(n * sizeof(T)));
    }

    return get_available_chunk()->allocate();
  }

  void deallocate(T *ptr, std::size_t n) noexcept {
    if (n != 1) {
      ::operator delete(ptr);
      return;
    }

    for (auto &chunk : std::views::reverse(chunks_)) {
      if (chunk->contains(ptr)) {
        chunk->deallocate(ptr);
        static thread_local std::size_t cleanup_counter = 0;
        if (++cleanup_counter >= 2 * ChunkSize) {
          cleanup_counter = 0;
          cleanup_empty_chunks();
        } else if (chunk->get_used() == ChunkSize - 1) {
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

template <typename T, std::size_t ChunkSize>
using ChunkedForwardList = std::forward_list<T, ChunkedAllocator<T, ChunkSize>>;

} // namespace l3::vm
