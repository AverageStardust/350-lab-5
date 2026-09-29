#include <iostream>
#include <optional>
#include <vector>

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include <SFML/Graphics.hpp>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int FPS_LIMIT = 30;

const float LINE_THICKNESS = 3;
const float CONTROL_RADIUS = 8;
const float SQUARE_SIZE = 20;
const float SQUARE_SPEED = 200; // px/s
const sf::Color CURVE_COLOR = sf::Color::Yellow;
const sf::Color CONTROL_COLOR = sf::Color::Blue;
const sf::Color SQUARE_COLOR = sf::Color::Green;

using Point2D = sf::Vector2f;

std::vector<Point2D> curve = {
    Point2D(WINDOW_WIDTH * 0.25, WINDOW_HEIGHT * 0.75),
    Point2D(WINDOW_WIDTH * 0.25, WINDOW_HEIGHT * -0.25),
    Point2D(WINDOW_WIDTH * 0.75, WINDOW_HEIGHT * 0.25),
    Point2D(WINDOW_WIDTH * 0.75, WINDOW_HEIGHT * 0.75),
};
float curveTime = 0;

Point2D lerp(const Point2D& a, const Point2D& b, float t) { return a + (b - a) * t; }

Point2D getPoint(const std::vector<Point2D>& pts, float t) {
    assert(pts.size() == 4);
    sf::Vector2f p1 = lerp(pts[0], pts[1], t);
    sf::Vector2f p2 = lerp(pts[1], pts[2], t);
    sf::Vector2f p3 = lerp(pts[2], pts[3], t);

    sf::Vector2f p4 = lerp(p1, p2, t);
    sf::Vector2f p5 = lerp(p2, p3, t);

    return lerp(p4, p5, t);
}

Point2D getSlope(const std::vector<Point2D>& pts, float t) {
    assert(pts.size() == 4);
    return 3 * (1 - t) * (1 - t) * (pts[1] - pts[0]) + 6 * t * (1 - t) * (pts[2] - pts[1]) +
           3 * t * t * (pts[3] - pts[2]);
}

// TODO: (Part 3) Track the index of the control point being dragged.

void handleInput(sf::Window& window, bool& shouldQuit) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            shouldQuit = true;
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            // TODO: (Part 3) On left-click, select the closest control point
            // using mouse->position and start dragging it.
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>()) {
            // TODO: (Part 3) On left-button release, stop dragging.
        } else if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
            // TODO: (Part 3) Move the selected control point to mouse->position.
            // TODO: (Part 4) Maintain matching slopes at shared endpoints.
            // When moving point 3, move point 5 without changing its distance
            // from point 4 (point numbers here start at 1).
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            // TODO: (Part 4) '+' adds three control points; '-' removes three,
            // keeping at least four points.
        }
    }
}

void renderBezier(sf::RenderWindow& window, const std::vector<Point2D>& pts) {
    assert(pts.size() == 4);

    sf::VertexArray curve(sf::PrimitiveType::LineStrip, 100);

    for (int i = 0; i < 100; i++) {
        float t = i / 99.0;
        curve[i].position = getPoint(pts, t);
        curve[i].color = CURVE_COLOR;
    }

    window.draw(curve);

    sf::VertexArray lines(sf::PrimitiveType::Lines, 4);

    for (int i = 0; i < 4; i++) {
        lines[i].position = pts[i];
        lines[i].color = CONTROL_COLOR;
    }

    window.draw(lines);

    for (int i = 0; i < 4; i++) {
        sf::CircleShape point(CONTROL_RADIUS);

        point.setPosition(pts[i] - Point2D(CONTROL_RADIUS, CONTROL_RADIUS));
        point.setFillColor(sf::Color::Transparent);
        point.setOutlineColor((i == 0 || i == 3) ? CURVE_COLOR : CONTROL_COLOR);
        point.setOutlineThickness(LINE_THICKNESS);

        window.draw(point);
    }
}

void renderSquare(sf::RenderWindow& window, const std::vector<Point2D>& pts, float t) {
    sf::RectangleShape square(Point2D(SQUARE_SIZE, SQUARE_SIZE));

    sf::Angle angle = getSlope(pts, t).angle();
    square.setPosition(getPoint(pts, t) +
                       Point2D(-SQUARE_SIZE, -SQUARE_SIZE).rotatedBy(angle) * 0.5f);
    square.setRotation(angle);
    square.setFillColor(sf::Color::Transparent);
    square.setOutlineColor(SQUARE_COLOR);
    square.setOutlineThickness(LINE_THICKNESS);

    window.draw(square);
}

void update(const std::vector<Point2D>& pts, float& t) {
    t += SQUARE_SPEED / FPS_LIMIT / getSlope(pts, t).length();
    while (t > 1.0) t--;
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);

    renderBezier(window, curve);
    renderSquare(window, curve, curveTime);

    // ====== ====== ======
    // TODO: (Part 3) Draw control handles from point 1 to 2 and point 3 to 4.
    // TODO: (Part 4) Draw all connected cubic Bezier segments and their handles.
    // ====== ====== ======

    // ====== ====== ======
    // TODO: (Bonus) Support multiple curves, a Galaga screen overlay at a 1:2 ratio, and exporting
    // curve points as C++ code for Project 1b.
    // ====== ====== ======

    window.display();

    update(curve, curveTime);
}

int main() {
    sf::RenderWindow window;

    try {
        // Initialize window
        window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Bezier Curve Editor");
        window.setFramerateLimit(FPS_LIMIT);
        // Prevent key repeats.
        window.setKeyRepeatEnabled(false);

        bool shouldQuit = false;
        // Main game loop
        while (window.isOpen()) {
            handleInput(window, shouldQuit);
            if (shouldQuit) {
                break;
            }
            render(window);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
