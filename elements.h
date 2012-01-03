#include <string>

class Keyword;
class Click;
class Explosion;
class Link;

class Keyword
{
    public:
    std::string kw;      // The string representation of the keyword
    double x[2];         // The location of the text
    double dr;           // recessional velocity
    double theta;        // The rotation of the text
    double alpha;        // Transperance
    double dalpha;       // How fast the alpha changes
    int count;
};

class Click
{
    public:
    double x[2];         // The location of the point
    double alpha;        // Transperance
    double dalpha;       // How fast the alpha changes
};

// Explosion surrounding a click event
class Explosion
{
    public:
    double x[2];         // The location of the point
    double r;            // Radius of the blast
    double dr;           // Recessional velocity
    double alpha;        // Transperance
    double dalpha;       // How fast the alpha changes

    Click *click;        // Link to the associated click
};

// A line linking an explosion linked to a blast
class Link
{
    public:
    Click *click;
    Keyword *kw;

    double alpha;        // Transperance
    double dalpha;       // How fast the alpha changes
};
