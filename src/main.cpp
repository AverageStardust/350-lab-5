#include <iostream>
#include <optional>
#include <vector>

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/System/Angle.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"
#include <SFML/Graphics.hpp>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int FPS_LIMIT = 30;

const float LINE_THICKNESS = 3;
const float CONTROL_RADIUS = 8;
const float SQUARE_SIZE = 20;
const float SQUARE_SPEED = 200;  // px/s
const float MAX_SELECT_DIST = 40;
const float ADD_SPACING = 30;
const sf::Color CURVE_COLOR = sf::Color::Yellow;
const sf::Color CONTROL_COLOR = sf::Color::Blue;
const sf::Color SQUARE_COLOR = sf::Color::Green;
const sf::Color BORDER_COLOR = sf::Color(100, 100, 100);

using Point2D = sf::Vector2f;

std::vector<Point2D> curve = {
    Point2D(WINDOW_WIDTH * 0.25, WINDOW_HEIGHT * 0.75),
    Point2D(WINDOW_WIDTH * 0.25, WINDOW_HEIGHT * 0.25),
    Point2D(WINDOW_WIDTH * 0.75, WINDOW_HEIGHT * 0.25),
    Point2D(WINDOW_WIDTH * 0.75, WINDOW_HEIGHT * 0.75),
};
int selectedPoint = -1;
float curveTime = 0;

Point2D lerp(const Point2D& a, const Point2D& b, float t) { return a + (b - a) * t; }

Point2D getPoint(std::vector<Point2D>::iterator segment, float t) {
    // cubic bezier points
    Point2D p0 = *segment;
    Point2D p1 = *(++segment);
    Point2D p2 = *(++segment);
    Point2D p3 = *(++segment);

    // quadratic bezier points
    Point2D q0 = lerp(p0, p1, t);
    Point2D q1 = lerp(p1, p2, t);
    Point2D q2 = lerp(p2, p3, t);

    // linear bezier points
    Point2D l0 = lerp(q0, q1, t);
    Point2D l1 = lerp(q1, q2, t);

    return lerp(l0, l1, t);
}

Point2D getSlope(std::vector<Point2D>::iterator segment, float t) {
    Point2D p0 = *segment;
    Point2D p1 = *(++segment);
    Point2D p2 = *(++segment);
    Point2D p3 = *(++segment);

    return 3 * (1 - t) * (1 - t) * (p1 - p0) + 6 * t * (1 - t) * (p2 - p1) + 3 * t * t * (p3 - p2);
}

void appendPoint(Point2D point) {
    point.x = fmax(0, fmin(WINDOW_WIDTH, point.x));
    point.y = fmax(0, fmin(WINDOW_WIDTH, point.y));
    curve.push_back(point);
}

void movePoint(int index, Point2D position) {
    if (index % 3 == 2 && index < curve.size() - 2) {
        curve[index] = position;
        curve[index + 2] = curve[index + 1] * 2.f - curve[index];
    } else if (index % 3 == 1 && index > 1) {
        curve[index] = position;
        curve[index - 2] = curve[index - 1] * 2.f - curve[index];
    } else if (index % 3 == 0 && index > 0 && index < curve.size() - 1) {
        Point2D movement = position - curve[index];
        curve[index - 1] += movement;
        curve[index] += movement;
        curve[index + 1] += movement;
    } else {
        curve[index] = position;
    }
}

void handleInput(sf::Window& window, bool& shouldQuit) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            shouldQuit = true;
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mouse->button == sf::Mouse::Button::Left) {
                float nearestDistSq = INFINITY;

                for (int i = 0; i < curve.size(); i++) {
                    float distSq = (curve[i] - Point2D(mouse->position)).lengthSquared();
                    if (distSq < nearestDistSq) {
                        nearestDistSq = distSq;
                        selectedPoint = i;
                    }
                }

                if (nearestDistSq > MAX_SELECT_DIST * MAX_SELECT_DIST) {
                    selectedPoint = -1;
                } else {
                    curve[selectedPoint] = Point2D(mouse->position);
                }
            }
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (mouse->button == sf::Mouse::Button::Left) {
                selectedPoint = -1;
            }
        } else if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
            if (selectedPoint != -1) {
                Point2D movement = Point2D(mouse->position) - curve[selectedPoint];
                movePoint(selectedPoint, Point2D(mouse->position));
            }
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->scancode == sf::Keyboard::Scan::Equal) {
                Point2D endPoint = curve[curve.size() - 1];
                Point2D direction = endPoint - curve[curve.size() - 2];

                if (direction != Point2D(0, 0)) {
                    direction = direction.normalized();

                    for (int i = 1; i < 4; i++) {
                        appendPoint(endPoint + direction * ADD_SPACING * float(i));
                    }

                    if (curve[curve.size() - 1] == curve[curve.size() - 2] ||
                        curve[curve.size() - 2] == curve[curve.size() - 3]) {
                        curve.resize(curve.size() - 3);
                    } else {
                        // update point to fix handles
                        movePoint(curve.size() - 3, curve[curve.size() - 3]);
                    }
                }

            } else if (key->scancode == sf::Keyboard::Scan::Hyphen) {
                if (curve.size() > 4) curve.resize(curve.size() - 3);
            }
        }
    }
}

void renderBezier(sf::RenderWindow& window, std::vector<Point2D>::iterator segment) {
    sf::VertexArray curve(sf::PrimitiveType::TriangleStrip, 200);

    for (int i = 0; i < 200; i += 2) {
        float t = i / 198.0;
        Point2D slope = getSlope(segment, t).normalized();
        Point2D lineNormal = slope.rotatedBy(sf::degrees(90));
        curve[i].position = getPoint(segment, t) + lineNormal * LINE_THICKNESS / 2.f;
        curve[i + 1].position = getPoint(segment, t) - lineNormal * LINE_THICKNESS / 2.f;
        curve[i].color = CURVE_COLOR;
        curve[i + 1].color = CURVE_COLOR;
    }

    window.draw(curve);

    sf::VertexArray lines(sf::PrimitiveType::TriangleStrip, 4);

    Point2D lineNormal;
    for (int i = 0; i < 8; i += 2) {
        if (i % 4 == 0) {
            Point2D slope = (*(segment + 1) - *segment).normalized();
            lineNormal = slope.rotatedBy(sf::degrees(90));
        }

        lines[i % 4].position = *segment + lineNormal * LINE_THICKNESS / 2.f;
        lines[(i + 1) % 4].position = *segment - lineNormal * LINE_THICKNESS / 2.f;
        lines[i % 4].color = CONTROL_COLOR;
        lines[(i + 1) % 4].color = CONTROL_COLOR;

        sf::CircleShape point(CONTROL_RADIUS);

        point.setPosition(*segment - Point2D(CONTROL_RADIUS, CONTROL_RADIUS));
        point.setFillColor(sf::Color::Transparent);
        point.setOutlineColor((i == 0 || i == 3) ? CURVE_COLOR : CONTROL_COLOR);
        point.setOutlineThickness(LINE_THICKNESS);

        window.draw(point);

        segment++;

        if (i % 4 == 2) {
            window.draw(lines);
        }
    }
}

void renderSquare(sf::RenderWindow& window, std::vector<Point2D>::iterator segment,
                  float segmentT) {
    sf::RectangleShape square(Point2D(SQUARE_SIZE, SQUARE_SIZE));

    Point2D slope = getSlope(segment, segmentT);

    sf::Angle angle = getSlope(segment, segmentT).angle();

    square.setPosition(getPoint(segment, segmentT) +
                       Point2D(-SQUARE_SIZE, -SQUARE_SIZE).rotatedBy(angle) * 0.5f);
    square.setRotation(angle);
    square.setFillColor(sf::Color::Transparent);
    square.setOutlineColor(SQUARE_COLOR);
    square.setOutlineThickness(LINE_THICKNESS);

    window.draw(square);
}

void renderBorder(sf::RenderWindow& window) {
    sf::RectangleShape border(Point2D(WINDOW_HEIGHT * 0.4, WINDOW_HEIGHT * 0.8));

    border.setPosition(Point2D(WINDOW_WIDTH * 0.5 - WINDOW_HEIGHT * 0.2, WINDOW_HEIGHT * 0.1));
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(BORDER_COLOR);
    border.setOutlineThickness(LINE_THICKNESS);

    window.draw(border);
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);

    float curveTimeIntegralPart;
    float segmentTime = std::modf(curveTime, &curveTimeIntegralPart);
    int segmentIndex = curveTimeIntegralPart;

    std::vector<Point2D>::iterator curveSegment = curve.begin() + segmentIndex * 3;

    renderBorder(window);

    for (auto segment = curve.begin(); segment + 1 != curve.end(); segment += 3) {
        renderBezier(window, segment);
    }

    renderSquare(window, curveSegment, segmentTime);

    window.display();

    float curveSlope = getSlope(curveSegment, segmentTime).length();
    curveTime += SQUARE_SPEED / FPS_LIMIT / curveSlope;

    float maxCurveTime = int((curve.size() - 1) / 3);
    while (curveTime > maxCurveTime) curveTime = 0;
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
