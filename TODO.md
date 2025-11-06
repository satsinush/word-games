- Add function descriptions and inline comments for confusing lines of code
- Add Strands solver?
- Add Waffle solver?

- Standardize output of solver to use Result structs
- Use pointers where appropriate to reduce copying overhead

- Fix debug heap corruption error and segmentation faults when using Tracy
  - Update Tracy to use proper thread handling