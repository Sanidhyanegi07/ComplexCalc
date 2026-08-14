#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QClipboard>
#include <QGuiApplication>
#include <QShortcut>

#include <complex>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>
#include <cctype>

using namespace std;
using Complex = complex<double>;

class ExpressionParser {
    struct Token {
        enum Type { Number, Name, Plus, Minus, Mul, Div, Pow, Fact,
                    Left, Right, Comma, End } type;
        string text;
        double value = 0;
    };

    vector<Token> tokens;
    size_t pos = 0;
    Complex answer{0, 0};

    static bool nameStart(char c) {
        return isalpha(static_cast<unsigned char>(c)) || c == '_';
    }

    static bool nameChar(char c) {
        return isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    vector<Token> tokenize(const string& s) {
        vector<Token> out;

        for (size_t i = 0; i < s.size();) {
            char c = s[i];

            if (isspace(static_cast<unsigned char>(c))) {
                ++i;
                continue;
            }

            if (isdigit(static_cast<unsigned char>(c)) || c == '.') {
                size_t start = i;
                bool dot = false;

                while (i < s.size()) {
                    char x = s[i];
                    if (isdigit(static_cast<unsigned char>(x))) {
                        ++i;
                    } else if (x == '.' && !dot) {
                        dot = true;
                        ++i;
                    } else {
                        break;
                    }
                }

                if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
                    size_t exponent = i++;
                    if (i < s.size() && (s[i] == '+' || s[i] == '-'))
                        ++i;

                    size_t digits = i;
                    while (i < s.size() &&
                           isdigit(static_cast<unsigned char>(s[i])))
                        ++i;

                    if (digits == i)
                        i = exponent;
                }

                string n = s.substr(start, i - start);
                out.push_back({Token::Number, n, stod(n)});
                continue;
            }

            if (nameStart(c)) {
                size_t start = i++;
                while (i < s.size() && nameChar(s[i]))
                    ++i;

                out.push_back({Token::Name, s.substr(start, i - start), 0});
                continue;
            }

            Token::Type type;

            switch (c) {
            case '+': type = Token::Plus; break;
            case '-': type = Token::Minus; break;
            case '*': type = Token::Mul; break;
            case '/': type = Token::Div; break;
            case '^': type = Token::Pow; break;
            case '!': type = Token::Fact; break;
            case '(': type = Token::Left; break;
            case ')': type = Token::Right; break;
            case ',': type = Token::Comma; break;
            case '%':
                out.push_back({Token::Number, "0.01", 0.01});
                ++i;
                continue;
            default:
                throw runtime_error("Invalid character");
            }

            out.push_back({type, string(1, c), 0});
            ++i;
        }

        out.push_back({Token::End, "", 0});
        return out;
    }

    bool is(Token::Type type) const {
        return tokens[pos].type == type;
    }

    void eat(Token::Type type) {
        if (!is(type))
            throw runtime_error("Invalid expression");
        ++pos;
    }

    bool startsPrimary() const {
        return is(Token::Number) || is(Token::Name) || is(Token::Left);
    }

    Complex expression() {
        Complex x = term();

        while (is(Token::Plus) || is(Token::Minus)) {
            if (is(Token::Plus)) {
                eat(Token::Plus);
                x += term();
            } else {
                eat(Token::Minus);
                x -= term();
            }
        }

        return x;
    }

    Complex term() {
        Complex x = power();

        while (true) {
            if (is(Token::Mul)) {
                eat(Token::Mul);
                x *= power();
            } else if (is(Token::Div)) {
                eat(Token::Div);
                Complex y = power();

                if (abs(y) < 1e-14)
                    throw runtime_error("Division by zero");

                x /= y;
            } else if (startsPrimary()) {
                x *= power();
            } else {
                break;
            }
        }

        return x;
    }

    Complex power() {
        Complex x = unary();

        if (is(Token::Pow)) {
            eat(Token::Pow);
            x = pow(x, power());
        }

        return x;
    }

    Complex unary() {
        if (is(Token::Plus)) {
            eat(Token::Plus);
            return unary();
        }

        if (is(Token::Minus)) {
            eat(Token::Minus);
            return -unary();
        }

        return postfix();
    }

    Complex postfix() {
        Complex x = primary();

        while (is(Token::Fact)) {
            eat(Token::Fact);

            if (abs(x.imag()) > 1e-12 ||
                x.real() < 0 ||
                floor(x.real()) != x.real() ||
                x.real() > 170)
                throw runtime_error("Invalid factorial");

            x = Complex(tgamma(x.real() + 1), 0);
        }

        return x;
    }

    Complex primary() {
        if (is(Token::Number)) {
            double value = tokens[pos].value;
            eat(Token::Number);

            if (is(Token::Name) &&
                (tokens[pos].text == "i" || tokens[pos].text == "j")) {
                eat(Token::Name);
                return Complex(0, value);
            }

            return Complex(value, 0);
        }

        if (is(Token::Left)) {
            eat(Token::Left);
            Complex x = expression();
            eat(Token::Right);
            return x;
        }

        if (is(Token::Name)) {
            string name = tokens[pos].text;
            eat(Token::Name);

            if (name == "pi")
                return Complex(M_PI, 0);

            if (name == "e")
                return Complex(M_E, 0);

            if (name == "i" || name == "j")
                return Complex(0, 1);

            if (name == "ans")
                return answer;

            if (is(Token::Left))
                return function(name);

            throw runtime_error("Unknown symbol");
        }

        throw runtime_error("Invalid expression");
    }

    Complex function(const string& name) {
        eat(Token::Left);

        vector<Complex> args;

        if (!is(Token::Right)) {
            args.push_back(expression());

            while (is(Token::Comma)) {
                eat(Token::Comma);
                args.push_back(expression());
            }
        }

        eat(Token::Right);

        if (name == "log" && args.size() == 2)
            return log(args[0]) / log(args[1]);

        if (args.size() != 1)
            throw runtime_error("Invalid arguments");

        Complex x = args[0];

        if (name == "sin") return sin(x);
        if (name == "cos") return cos(x);
        if (name == "tan") return tan(x);
        if (name == "asin") return asin(x);
        if (name == "acos") return acos(x);
        if (name == "atan") return atan(x);
        if (name == "sinh") return sinh(x);
        if (name == "cosh") return cosh(x);
        if (name == "tanh") return tanh(x);
        if (name == "sqrt") return sqrt(x);
        if (name == "cbrt") return pow(x, Complex(1.0 / 3.0, 0));
        if (name == "exp") return exp(x);
        if (name == "ln") return log(x);
        if (name == "log") return log10(x);
        if (name == "abs") return Complex(abs(x), 0);
        if (name == "arg") return Complex(arg(x), 0);
        if (name == "re") return Complex(x.real(), 0);
        if (name == "im") return Complex(x.imag(), 0);
        if (name == "conj") return conj(x);
        if (name == "floor") return Complex(floor(x.real()), 0);
        if (name == "ceil") return Complex(ceil(x.real()), 0);
        if (name == "round") return Complex(round(x.real()), 0);

        throw runtime_error("Unknown function");
    }

public:
    static string format(Complex z) {
        double real = abs(z.real()) < 1e-10 ? 0 : z.real();
        double imag = abs(z.imag()) < 1e-10 ? 0 : z.imag();

        auto formatNumber = [](double x) {
            ostringstream out;
            out << setprecision(12) << x;
            return out.str();
        };

        if (imag == 0)
            return formatNumber(real);

        if (real == 0)
            return formatNumber(imag) + "i";

        return formatNumber(real) +
               (imag >= 0 ? " + " : " - ") +
               formatNumber(abs(imag)) + "i";
    }

    string evaluate(const string& expressionText) {
        if (expressionText.empty())
            throw runtime_error("Enter an expression");

        tokens = tokenize(expressionText);
        pos = 0;

        Complex result = expression();

        if (!is(Token::End))
            throw runtime_error("Invalid expression");

        answer = result;
        return format(result);
    }
};

class ComplexCalc : public QMainWindow {
    QStackedWidget* pages = nullptr;
    QLineEdit* input = nullptr;
    QLabel* result = nullptr;
    QLabel* state = nullptr;
    QListWidget* history = nullptr;

    QComboBox* category = nullptr;
    QComboBox* fromUnit = nullptr;
    QComboBox* toUnit = nullptr;
    QDoubleSpinBox* convertValue = nullptr;
    QLabel* convertResult = nullptr;

    QComboBox* everyday = nullptr;
    QDoubleSpinBox* firstValue = nullptr;
    QDoubleSpinBox* secondValue = nullptr;
    QDoubleSpinBox* thirdValue = nullptr;
    QLabel* firstLabel = nullptr;
    QLabel* secondLabel = nullptr;
    QLabel* thirdLabel = nullptr;
    QLabel* everydayResult = nullptr;

    ExpressionParser parser;

    QPushButton* makeButton(const QString& text,
                            const QString& type = "") {
        auto* b = new QPushButton(text);
        b->setProperty("type", type);
        b->setFocusPolicy(Qt::NoFocus);
        return b;
    }

    QLabel* makeTitle(const QString& text) {
        auto* l = new QLabel(text);
        l->setObjectName("title");
        return l;
    }

    void calculate() {
        QString text = input->text().trimmed();

        if (text.isEmpty())
            return;

        try {
            QString value =
                QString::fromStdString(parser.evaluate(text.toStdString()));

            result->setText(value);
            state->setText("READY");
            history->insertItem(0, text + "   =   " + value);

            while (history->count() > 100)
                delete history->takeItem(100);

        } catch (const exception& e) {
            state->setText(QString("ERROR  •  ") + e.what());
        }
    }

    void insertKey(const QString& key) {
        if (key == "AC") {
            input->clear();
            result->setText("0");
            state->setText("READY");
            return;
        }

        if (key == "⌫") {
            input->backspace();
            return;
        }

        if (key == "=") {
            calculate();
            return;
        }

        QString value = key;

        if (key == "×") value = "*";
        if (key == "÷") value = "/";
        if (key == "−") value = "-";
        if (key == "√") value = "sqrt(";
        if (key == "x²") value = "^2";
        if (key == "xʸ") value = "^";

        const QStringList functions = {
            "sin","cos","tan","asin","acos","atan",
            "sinh","cosh","tanh","ln","log","sqrt",
            "cbrt","abs","arg","re","im","conj",
            "floor","ceil","round"
        };

        if (functions.contains(key))
            value += "(";

        input->insert(value);
        input->setFocus();
    }

    QStringList getUnits(const QString& type) {
        if (type == "Length")
            return {"Millimeter","Centimeter","Meter","Kilometer",
                    "Inch","Foot","Yard","Mile"};

        if (type == "Weight")
            return {"Milligram","Gram","Kilogram","Pound","Ounce"};

        if (type == "Temperature")
            return {"Celsius","Fahrenheit","Kelvin"};

        if (type == "Area")
            return {"Square Centimeter","Square Meter",
                    "Square Kilometer","Square Foot","Acre"};

        if (type == "Volume")
            return {"Milliliter","Liter","Cubic Meter","Gallon","Cup"};

        if (type == "Speed")
            return {"Meter/Second","Kilometer/Hour",
                    "Mile/Hour","Foot/Second"};

        return {"Second","Minute","Hour","Day"};
    }

    double toBase(const QString& type,
                  const QString& unit,
                  double value) {
        if (type == "Length") {
            if (unit == "Millimeter") return value / 1000;
            if (unit == "Centimeter") return value / 100;
            if (unit == "Kilometer") return value * 1000;
            if (unit == "Inch") return value * 0.0254;
            if (unit == "Foot") return value * 0.3048;
            if (unit == "Yard") return value * 0.9144;
            if (unit == "Mile") return value * 1609.344;
        }

        if (type == "Weight") {
            if (unit == "Milligram") return value / 1000000;
            if (unit == "Gram") return value / 1000;
            if (unit == "Pound") return value * 0.45359237;
            if (unit == "Ounce") return value * 0.028349523125;
        }

        if (type == "Temperature") {
            if (unit == "Fahrenheit") return (value - 32) * 5 / 9;
            if (unit == "Kelvin") return value - 273.15;
        }

        if (type == "Area") {
            if (unit == "Square Centimeter") return value / 10000;
            if (unit == "Square Kilometer") return value * 1000000;
            if (unit == "Square Foot") return value * 0.09290304;
            if (unit == "Acre") return value * 4046.8564224;
        }

        if (type == "Volume") {
            if (unit == "Milliliter") return value / 1000000;
            if (unit == "Liter") return value / 1000;
            if (unit == "Gallon") return value * 0.003785411784;
            if (unit == "Cup") return value * 0.0002365882365;
        }

        if (type == "Speed") {
            if (unit == "Kilometer/Hour") return value / 3.6;
            if (unit == "Mile/Hour") return value * 0.44704;
            if (unit == "Foot/Second") return value * 0.3048;
        }

        if (unit == "Minute") return value * 60;
        if (unit == "Hour") return value * 3600;
        if (unit == "Day") return value * 86400;

        return value;
    }

    double fromBase(const QString& type,
                    const QString& unit,
                    double value) {
        if (type == "Length") {
            if (unit == "Millimeter") return value * 1000;
            if (unit == "Centimeter") return value * 100;
            if (unit == "Kilometer") return value / 1000;
            if (unit == "Inch") return value / 0.0254;
            if (unit == "Foot") return value / 0.3048;
            if (unit == "Yard") return value / 0.9144;
            if (unit == "Mile") return value / 1609.344;
        }

        if (type == "Weight") {
            if (unit == "Milligram") return value * 1000000;
            if (unit == "Gram") return value * 1000;
            if (unit == "Pound") return value / 0.45359237;
            if (unit == "Ounce") return value / 0.028349523125;
        }

        if (type == "Temperature") {
            if (unit == "Fahrenheit") return value * 9 / 5 + 32;
            if (unit == "Kelvin") return value + 273.15;
        }

        if (type == "Area") {
            if (unit == "Square Centimeter") return value * 10000;
            if (unit == "Square Kilometer") return value / 1000000;
            if (unit == "Square Foot") return value / 0.09290304;
            if (unit == "Acre") return value / 4046.8564224;
        }

        if (type == "Volume") {
            if (unit == "Milliliter") return value * 1000000;
            if (unit == "Liter") return value * 1000;
            if (unit == "Gallon") return value / 0.003785411784;
            if (unit == "Cup") return value / 0.0002365882365;
        }

        if (type == "Speed") {
            if (unit == "Kilometer/Hour") return value * 3.6;
            if (unit == "Mile/Hour") return value / 0.44704;
            if (unit == "Foot/Second") return value / 0.3048;
        }

        if (unit == "Minute") return value / 60;
        if (unit == "Hour") return value / 3600;
        if (unit == "Day") return value / 86400;

        return value;
    }

    void refreshUnits() {
        QStringList list = getUnits(category->currentText());

        fromUnit->clear();
        toUnit->clear();

        fromUnit->addItems(list);
        toUnit->addItems(list);

        if (list.size() > 1)
            toUnit->setCurrentIndex(1);
    }

    void refreshEveryday() {
        QString type = everyday->currentText();

        thirdValue->setVisible(false);
        thirdLabel->setVisible(false);

        if (type == "BMI") {
            firstLabel->setText("Height (cm)");
            secondLabel->setText("Weight (kg)");
        } else if (type == "Percentage") {
            firstLabel->setText("Value");
            secondLabel->setText("Percent");
        } else if (type == "Discount") {
            firstLabel->setText("Price");
            secondLabel->setText("Discount %");
        } else if (type == "Tip") {
            firstLabel->setText("Bill");
            secondLabel->setText("Tip %");
        } else if (type == "Simple Interest" ||
                   type == "Compound Interest") {
            firstLabel->setText("Principal");
            secondLabel->setText("Rate %");
            thirdLabel->setText("Years");
            thirdLabel->setVisible(true);
            thirdValue->setVisible(true);
        } else {
            firstLabel->setText("Birth Year");
            secondLabel->setText("Current Year");
        }
    }

    QWidget* calculatorPage() {
        auto* page = new QWidget;
        auto* root = new QVBoxLayout(page);

        root->setContentsMargins(28, 24, 28, 24);
        root->setSpacing(16);

        auto* header = new QHBoxLayout;
        header->addWidget(makeTitle("Calculator"));
        header->addStretch();

        auto* badge = new QLabel("SCIENTIFIC  •  COMPLEX");
        badge->setObjectName("badge");
        header->addWidget(badge);
        root->addLayout(header);

        auto* display = new QFrame;
        display->setObjectName("display");

        auto* displayLayout = new QVBoxLayout(display);
        displayLayout->setContentsMargins(18, 12, 18, 12);

        input = new QLineEdit;
        input->setObjectName("input");
        input->setAlignment(Qt::AlignRight);

        result = new QLabel("0");
        result->setObjectName("result");
        result->setAlignment(Qt::AlignRight);

        state = new QLabel("READY");
        state->setObjectName("state");
        state->setAlignment(Qt::AlignRight);

        displayLayout->addWidget(input);
        displayLayout->addWidget(result);
        displayLayout->addWidget(state);

        root->addWidget(display);

        auto* content = new QHBoxLayout;
        content->setSpacing(16);

        auto* keypadFrame = new QFrame;
        keypadFrame->setObjectName("panel");

        auto* grid = new QGridLayout(keypadFrame);
        grid->setSpacing(8);

        QString keys[7][7] = {
            {"AC","⌫","(",")","%","!","÷"},
            {"sin","cos","tan","asin","acos","atan","×"},
            {"sinh","cosh","tanh","ln","log","√","−"},
            {"7","8","9","π","e","i","+"},
            {"4","5","6","x²","xʸ","Ans","="},
            {"1","2","3","abs","arg","conj","."},
            {"0","00","re","im","cbrt","floor","ceil"}
        };

        for (int r = 0; r < 7; ++r) {
            for (int c = 0; c < 7; ++c) {
                QString key = keys[r][c];
                QString type;

                if (key == "=")
                    type = "equal";
                else if (key == "AC" || key == "⌫")
                    type = "danger";
                else if (QString("+-×÷%^!").contains(key))
                    type = "operator";
                else if (key == "sin" || key == "cos" ||
                         key == "tan" || key == "asin" ||
                         key == "acos" || key == "atan" ||
                         key == "sinh" || key == "cosh" ||
                         key == "tanh" || key == "ln" ||
                         key == "log" || key == "√" ||
                         key == "x²" || key == "xʸ" ||
                         key == "abs" || key == "arg" ||
                         key == "conj" || key == "re" ||
                         key == "im" || key == "cbrt" ||
                         key == "floor" || key == "ceil")
                    type = "function";

                auto* b = makeButton(key, type);
                grid->addWidget(b, r, c);

                connect(b, &QPushButton::clicked,
                        this, [this, key] { insertKey(key); });
            }
        }

        content->addWidget(keypadFrame, 3);

        auto* historyFrame = new QFrame;
        historyFrame->setObjectName("panel");

        auto* historyLayout = new QVBoxLayout(historyFrame);

        auto* historyTitle = new QLabel("History");
        historyTitle->setObjectName("cardTitle");

        history = new QListWidget;

        auto* copy = makeButton("Copy Result");
        auto* clear = makeButton("Clear", "danger");

        historyLayout->addWidget(historyTitle);
        historyLayout->addWidget(history);
        historyLayout->addWidget(copy);
        historyLayout->addWidget(clear);

        content->addWidget(historyFrame, 2);
        root->addLayout(content, 1);

        connect(input, &QLineEdit::returnPressed,
                this, [this] { calculate(); });

        connect(copy, &QPushButton::clicked,
                this, [this] {
                    QGuiApplication::clipboard()->setText(result->text());
                });

        connect(clear, &QPushButton::clicked,
                this, [this] {
                    history->clear();
                });

        connect(history, &QListWidget::itemDoubleClicked,
                this, [this](QListWidgetItem* item) {
                    QString text = item->text();
                    int index = text.indexOf("   =   ");

                    if (index > 0)
                        input->setText(text.left(index));
                });

        return page;
    }

    QWidget* converterPage() {
        auto* page = new QWidget;
        auto* root = new QVBoxLayout(page);

        root->setContentsMargins(28, 24, 28, 24);
        root->addWidget(makeTitle("Converter"));

        auto* panel = new QFrame;
        panel->setObjectName("panel");

        auto* form = new QFormLayout(panel);

        category = new QComboBox;
        category->addItems({
            "Length","Weight","Temperature",
            "Area","Volume","Speed","Time"
        });

        convertValue = new QDoubleSpinBox;
        convertValue->setRange(-1e12, 1e12);
        convertValue->setDecimals(10);

        fromUnit = new QComboBox;
        toUnit = new QComboBox;

        convertResult = new QLabel("—");
        convertResult->setObjectName("resultBox");

        auto* convert = makeButton("Convert", "equal");
        auto* swap = makeButton("⇄", "operator");

        auto* buttons = new QHBoxLayout;
        buttons->addWidget(swap);
        buttons->addWidget(convert);

        form->addRow("Category", category);
        form->addRow("Value", convertValue);
        form->addRow("From", fromUnit);
        form->addRow("To", toUnit);
        form->addRow(buttons);
        form->addRow("Result", convertResult);

        root->addWidget(panel);
        root->addStretch();

        connect(category, &QComboBox::currentTextChanged,
                this, [this] { refreshUnits(); });

        connect(swap, &QPushButton::clicked,
                this, [this] {
                    int from = fromUnit->currentIndex();
                    int to = toUnit->currentIndex();
                    fromUnit->setCurrentIndex(to);
                    toUnit->setCurrentIndex(from);
                });

        connect(convert, &QPushButton::clicked,
                this, [this] {
                    QString type = category->currentText();

                    double baseValue =
                        toBase(type, fromUnit->currentText(),
                               convertValue->value());

                    double converted =
                        fromBase(type, toUnit->currentText(),
                                 baseValue);

                    convertResult->setText(
                        QString::number(converted, 'g', 12) +
                        "  " + toUnit->currentText());
                });

        refreshUnits();
        return page;
    }

    QWidget* everydayPage() {
        auto* page = new QWidget;
        auto* root = new QVBoxLayout(page);

        root->setContentsMargins(28, 24, 28, 24);
        root->addWidget(makeTitle("Everyday"));

        auto* panel = new QFrame;
        panel->setObjectName("panel");

        auto* form = new QFormLayout(panel);

        everyday = new QComboBox;
        everyday->addItems({
            "BMI","Percentage","Discount","Tip",
            "Simple Interest","Compound Interest","Age"
        });

        firstValue = new QDoubleSpinBox;
        secondValue = new QDoubleSpinBox;
        thirdValue = new QDoubleSpinBox;

        for (auto* box : {firstValue, secondValue, thirdValue}) {
            box->setRange(-1e12, 1e12);
            box->setDecimals(4);
        }

        firstLabel = new QLabel;
        secondLabel = new QLabel;
        thirdLabel = new QLabel;

        everydayResult = new QLabel("—");
        everydayResult->setObjectName("resultBox");

        auto* calculateButton = makeButton("Calculate", "equal");

        form->addRow("Calculation", everyday);
        form->addRow(firstLabel, firstValue);
        form->addRow(secondLabel, secondValue);
        form->addRow(thirdLabel, thirdValue);
        form->addRow(calculateButton);
        form->addRow("Result", everydayResult);

        root->addWidget(panel);
        root->addStretch();

        connect(everyday, &QComboBox::currentTextChanged,
                this, [this] { refreshEveryday(); });

        connect(calculateButton, &QPushButton::clicked,
                this, [this] {
                    QString type = everyday->currentText();

                    double A = firstValue->value();
                    double B = secondValue->value();
                    double C = thirdValue->value();

                    if (type == "BMI") {
                        if (A <= 0 || B <= 0) {
                            everydayResult->setText("Invalid values");
                            return;
                        }

                        double bmi = B / pow(A / 100.0, 2);

                        QString categoryName;

                        if (bmi < 18.5)
                            categoryName = "Underweight";
                        else if (bmi < 25)
                            categoryName = "Normal";
                        else if (bmi < 30)
                            categoryName = "Overweight";
                        else
                            categoryName = "Obesity";

                        everydayResult->setText(
                            QString("BMI  %1   •   %2")
                                .arg(bmi, 0, 'f', 2)
                                .arg(categoryName));
                    }
                    else if (type == "Percentage") {
                        everydayResult->setText(
                            QString::number(A * B / 100.0,
                                            'f', 2));
                    }
                    else if (type == "Discount") {
                        double saving = A * B / 100.0;
                        double finalPrice = A - saving;

                        everydayResult->setText(
                            QString("Saving  %1   •   Final  %2")
                                .arg(saving, 0, 'f', 2)
                                .arg(finalPrice, 0, 'f', 2));
                    }
                    else if (type == "Tip") {
                        double tip = A * B / 100.0;
                        double total = A + tip;

                        everydayResult->setText(
                            QString("Tip  %1   •   Total  %2")
                                .arg(tip, 0, 'f', 2)
                                .arg(total, 0, 'f', 2));
                    }
                    else if (type == "Simple Interest") {
                        double interest = A * B * C / 100.0;

                        everydayResult->setText(
                            QString("Interest  %1   •   Total  %2")
                                .arg(interest, 0, 'f', 2)
                                .arg(A + interest, 0, 'f', 2));
                    }
                    else if (type == "Compound Interest") {
                        double total =
                            A * pow(1 + B / 100.0, C);

                        everydayResult->setText(
                            QString("Interest  %1   •   Total  %2")
                                .arg(total - A, 0, 'f', 2)
                                .arg(total, 0, 'f', 2));
                    }
                    else {
                        int age =
                            static_cast<int>(B) -
                            static_cast<int>(A);

                        everydayResult->setText(
                            QString::number(age) + " years");
                    }
                });

        refreshEveryday();
        return page;
    }

    QWidget* aboutPage() {
        auto* page = new QWidget;
        auto* root = new QVBoxLayout(page);

        root->setContentsMargins(28, 24, 28, 24);
        root->addWidget(makeTitle("About"));

        auto* panel = new QFrame;
        panel->setObjectName("panel");

        auto* layout = new QVBoxLayout(panel);

        auto* name = new QLabel("COMPLEXCALC");
        name->setObjectName("aboutHead");

        auto* subtitle =
            new QLabel("Advanced mathematical desktop utility");
        subtitle->setObjectName("aboutSub");

        auto* features = new QLabel(
            "Scientific mathematics\n"
            "Complex numbers\n"
            "Expression parsing\n"
            "Unit conversion\n"
            "Everyday calculations\n"
            "Calculation history\n\n"
            "C++17  •  Qt Widgets");

        features->setObjectName("aboutText");

        layout->addWidget(name);
        layout->addWidget(subtitle);
        layout->addSpacing(14);
        layout->addWidget(features);
        layout->addStretch();

        root->addWidget(panel);
        root->addStretch();

        return page;
    }

    void applyStyle() {
        setStyleSheet(R"(
            QMainWindow, QWidget {
                background: #0a0d13;
                color: #edf1f8;
                font-family: "Segoe UI";
                font-size: 14px;
            }

            #side {
                background: #0d1119;
                border-right: 1px solid #202735;
            }

            #logo {
                font-size: 22px;
                font-weight: 800;
                color: #8d9aff;
            }

            #sideSub {
                color: #626d80;
                font-size: 11px;
            }

            #sideFoot {
                color: #465064;
                font-size: 11px;
            }

            #title {
                font-size: 28px;
                font-weight: 750;
                color: #ffffff;
            }

            #badge {
                background: #171f36;
                border: 1px solid #2c3960;
                border-radius: 9px;
                padding: 7px 11px;
                color: #91a0ff;
                font-size: 10px;
                font-weight: 700;
            }

            #display, #panel {
                background: #111722;
                border: 1px solid #222c3d;
                border-radius: 18px;
            }

            #input {
                border: 0;
                background: transparent;
                color: #8e9dff;
                font-size: 17px;
                padding: 8px;
            }

            #result {
                color: #ffffff;
                font-size: 38px;
                font-weight: 700;
                padding: 7px;
            }

            #state {
                color: #59d8b7;
                font-size: 10px;
                font-weight: 700;
                padding: 3px;
            }

            #cardTitle {
                font-size: 16px;
                font-weight: 700;
                color: #dfe5f0;
            }

            QPushButton {
                background: #171e2b;
                border: 1px solid #283348;
                border-radius: 10px;
                color: #e7ebf4;
                min-height: 43px;
                font-weight: 650;
            }

            QPushButton:hover {
                background: #222c3e;
                border-color: #6677ff;
            }

            QPushButton[type="operator"] {
                background: #192542;
                color: #94a5ff;
            }

            QPushButton[type="function"] {
                background: #142b2b;
                color: #5ed9bd;
            }

            QPushButton[type="equal"] {
                background: #5967f2;
                border: 0;
                color: #ffffff;
            }

            QPushButton[type="equal"]:hover {
                background: #6977ff;
            }

            QPushButton[type="danger"] {
                background: #2a1b24;
                color: #ff8398;
            }

            QComboBox, QDoubleSpinBox {
                background: #151c28;
                border: 1px solid #2b374b;
                border-radius: 9px;
                padding: 9px;
                color: #ffffff;
            }

            QListWidget {
                background: #0e141d;
                border: 1px solid #263145;
                border-radius: 12px;
            }

            QListWidget::item {
                padding: 10px;
                border-radius: 7px;
                color: #bdc6d5;
            }

            QListWidget::item:hover {
                background: #1b2433;
            }

            QListWidget::item:selected {
                background: #303c68;
                color: #ffffff;
            }

            #resultBox {
                background: #101c1c;
                border: 1px solid #245048;
                border-radius: 11px;
                padding: 16px;
                color: #62ddbf;
                font-size: 17px;
                font-weight: 700;
            }

            #aboutHead {
                font-size: 32px;
                font-weight: 800;
                color: #ffffff;
            }

            #aboutSub {
                font-size: 16px;
                color: #778399;
            }

            #aboutText {
                font-size: 16px;
                color: #c0c8d6;
                padding: 10px;
            }
        )");
    }

public:
    ComplexCalc() {
        setWindowTitle("ComplexCalc");
        resize(1280, 800);
        setMinimumSize(1050, 680);

        auto* central = new QWidget;
        auto* mainLayout = new QHBoxLayout(central);

        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        auto* side = new QWidget;
        side->setObjectName("side");
        side->setFixedWidth(220);

        auto* sideLayout = new QVBoxLayout(side);
        sideLayout->setContentsMargins(20, 28, 20, 20);
        sideLayout->setSpacing(6);

        auto* logo = new QLabel("◆ COMPLEXCALC");
        logo->setObjectName("logo");

        auto* sub = new QLabel("MATHEMATICAL UTILITY");
        sub->setObjectName("sideSub");

        sideLayout->addWidget(logo);
        sideLayout->addWidget(sub);
        sideLayout->addSpacing(30);

        pages = new QStackedWidget;

        pages->addWidget(calculatorPage());
        pages->addWidget(converterPage());
        pages->addWidget(everydayPage());
        pages->addWidget(aboutPage());

        QStringList navigation = {
            "Calculator",
            "Converter",
            "Everyday",
            "About"
        };

        for (int i = 0; i < navigation.size(); ++i) {
            auto* button = makeButton(navigation[i], "nav");
            sideLayout->addWidget(button);

            connect(button, &QPushButton::clicked,
                    this, [this, i] {
                        pages->setCurrentIndex(i);
                    });
        }

        sideLayout->addStretch();

        auto* footer = new QLabel("C++17  •  Qt Widgets");
        footer->setObjectName("sideFoot");
        sideLayout->addWidget(footer);

        mainLayout->addWidget(side);
        mainLayout->addWidget(pages, 1);

        setCentralWidget(central);
        applyStyle();

        auto* shortcut =
            new QShortcut(QKeySequence(Qt::Key_Return), this);

        connect(shortcut, &QShortcut::activated,
                this, [this] {
                    calculate();
                });
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("ComplexCalc");
    app.setApplicationVersion("3.0");

    ComplexCalc window;
    window.show();

    return app.exec();
}