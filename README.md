# ProFor — Fortran 90 Oracle Database Integration

**ProFor** is a program that enables **Fortran 90 applications** to access **Oracle databases** using the traditional **Oracle Pro*FORTRAN embedded SQL model**.  
It provides a structured way to execute SQL statements, call PL/SQL stored procedures, control transactions, and retrieve data from Oracle databases in scientific, engineering, and legacy systems.

ProFor follows the classic Oracle approach where **SQL is embedded directly in the Fortran source code** using `EXEC SQL` directives, which are later processed and translated into C code compatible with Oracle client libraries.

---

## Key Features

- Oracle database access from **Fortran 90**
- Support for **embedded SQL (`EXEC SQL`)**
- Execution of **PL/SQL stored procedures and functions**
- Explicit transaction control (COMMIT / ROLLBACK)
- Cursor-based data retrieval
- Designed for **batch processing and scientific applications**
- Compatible with the traditional **Oracle Pro*FORTRAN toolchain**

---

## Development Model (Oracle-Compatible)

ProFor follows the historical Oracle workflow for Fortran database applications:

1. **Write database logic in Fortran 90 using Pro*FORTRAN embedded SQL syntax**
2. Convert the Fortran source code to C using `f2c`
3. Preprocess embedded SQL using **Oracle PRO*C**
4. Compile and link against Oracle client libraries
5. Execute the resulting binary

This approach mirrors the original Oracle-supported method for integrating Fortran applications with Oracle databases.

---

## Requirements

### Code Conversion
- **f2c**
  - Required to translate Fortran 90 source code into C
  - Used as an intermediate step before SQL preprocessing

### Oracle Integration
- **Oracle PRO*C**
  - Required to preprocess embedded SQL (`EXEC SQL`) statements
  - Generates Oracle-compatible C source code

### Compilation
- C compiler compatible with Oracle client libraries
- Oracle client environment properly configured

---

## Portability Considerations

Although ProFor is designed around Oracle’s Pro*FORTRAN / PRO*C toolchain,  
the **generated C code can be adapted to other database platforms** with relative ease, enabling future migration away from Oracle if required.

---

## Typical Use Cases

- Legacy scientific and engineering systems written in Fortran
- Batch-processing applications requiring direct database access
- Systems originally designed around Oracle Pro*FORTRAN environments
- Long-lived numerical codes requiring minimal architectural changes

---

## Notes

- ProFor preserves compatibility with **historical Oracle embedded SQL conventions**
- It is intended for environments where **Fortran remains a core language**
- The project focuses on integration stability rather than modern ORM-style abstractions
