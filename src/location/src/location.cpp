module l3.location;

namespace l3::location {

void Position::initialize(filename_type fn, counter_type l, counter_type c) {
  filename = fn;
  line = l;
  column = c;
}

void Position::lines(counter_type count) {
  if (count != 0) {
    column = 1;
    line = add_(line, count, 1);
  }
}

void Location::initialize(filename_type f, counter_type l, counter_type c) {
  begin.initialize(f, l, c);
  end = begin;
}

} // namespace l3::location
