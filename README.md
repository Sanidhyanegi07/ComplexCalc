# ComplexCalc

## Advanced C++ Qt GUI Calculator

ComplexCalc is a modern desktop calculator built entirely with **C++ and Qt Widgets**. It combines an advanced mathematical engine with a clean, vivid graphical interface and useful everyday calculation tools.

The project is designed as a single-file C++ application with CMake support, making the core application easy to understand, build, and modify.

---

## ✨ Features

### 🧮 Advanced Calculator

- Basic arithmetic operations
- Scientific calculations
- Expression parsing
- Implicit multiplication
- Powers and roots
- Factorials
- Percent calculations
- Mathematical constants such as π and e
- Previous answer support
- Trigonometric functions
- Inverse trigonometric functions
- Hyperbolic functions
- Logarithmic and exponential functions
- Complex-number calculations
- Real and imaginary component extraction
- Complex conjugate
- Complex magnitude and argument
- Floor, ceiling and rounding operations

### 🔢 Complex Mathematics

The calculator supports complex numbers using:

- `i`
- Real numbers
- Complex arithmetic
- Complex powers
- Complex roots
- Complex trigonometric functions
- Complex logarithmic functions

### 📜 Calculation History

- Stores previous calculations
- Displays expressions and results
- Double-click a history entry to reuse its expression
- Copy results to the clipboard
- Clear calculation history

### 🔄 Converter

The Converter section supports common real-world conversions:

- Length
- Weight
- Temperature
- Area
- Volume
- Speed
- Time

It also includes a quick unit swap function.

### 📊 Everyday Calculations

Useful calculations for everyday situations:

- BMI
- Percentage
- Discount
- Tip
- Simple Interest
- Compound Interest
- Age calculation

### 🎨 Modern GUI

- Clean dark interface
- Vivid accent elements
- Organized navigation
- Scientific keypad
- Responsive calculator layout
- Separate Calculator, Converter, Everyday and About sections
- Keyboard support
- Easy-to-use controls

---

## 🛠️ Technology

- **C++17**
- **Qt Widgets**
- **CMake**
- Standard C++ Library
- `<complex>` for complex-number mathematics

The graphical interface is created using Qt, while the mathematical logic and application functionality are implemented in C++.

---

## 📁 Project Structure

```text
ComplexCalc/
│
├── main.cpp
├── CMakeLists.txt
├── README.md
└── .gitignore
