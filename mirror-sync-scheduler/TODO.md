# Todos

    - Update private class members to use `m_` prefix.
    - Convert public members to private members where possible.
    - Other todos listed in code (`rg -i TODO`).
    - Restructure classes to not use singleton design pattern.
    - Move doxygen to the function declarations instead of the definitions
    - Convert line-comment doxygen to comment blocks
    - Start using const reference where reasonable instead of passing by value.
    - There are a lot of loose threads being managed in so many different 
      contexts. I think there should be a thread pool to manage all threads 
      other than the main thread
    - Use bounds checked access where possible (replace `[]` with `.at()`).
    - Extract LCM determination to a function (Use `std::lcm()`?)
    - There are a lot of pointers in use that are probably not needed. They 
      should either be replaced entirely by things that are not pointers or they
      should be replaced with smart pointers (`std::shared_ptr`, 
      `std::unique_ptr`, and `std::weak_ptr`)
    - Where possible replace use of `int` with more descriptive types 
      (`std::size_t`, `std::uint64_t`, `std::byte`, ...)
    - Wrap everything in `namespace mirror::sync_scheduler::<insert_name_here>`
    - Revisit `Job` and `Task` structs
    - Shrink `main()` massively.
    - Take everything that isn't `main()` out of `main.cpp`
    - There are global variables that could probably not exist.
    - Make things const where possible
    - Make things constexpr where possible.
