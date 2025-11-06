- Standardize output of solver to use Result structs
- Use pointers where appropriate to reduce copying overhead

- Fix debug heap corruption error and segmentation faults when using Tracy
  - Update Tracy to use proper thread handling