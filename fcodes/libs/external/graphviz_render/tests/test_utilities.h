#pragma once

#include <filesystem>
#include <string.h>
#include <string>
#include <string_view>
#include <unordered_set>

/// node shapes

extern const std::unordered_set<std::string_view> all_node_shapes;
extern const std::unordered_set<std::string_view>
    node_shapes_consisting_of_ellipse;
extern const std::unordered_set<std::string_view>
    node_shapes_consisting_of_ellipse_and_polyline;
extern const std::unordered_set<std::string_view>
    node_shapes_consisting_of_path;
extern const std::unordered_set<std::string_view>
    node_shapes_consisting_of_polygon;
extern const std::unordered_set<std::string_view>
    node_shapes_consisting_of_polygon_and_polyline;
extern const std::unordered_set<std::string_view> node_shapes_without_svg_shape;

bool contains_ellipse_shape(std::string_view shape);
bool contains_multiple_shapes_with_different_fill(std::string_view shape);
bool contains_polygon_shape(std::string_view shape);
bool contains_polyline_shape(std::string_view shape);

bool is_polygon_shape(std::string_view shape);

/// arrow shapes
extern const std::unordered_set<std::string_view>
    primitive_polygon_arrow_shapes;
extern const std::unordered_set<std::string_view>
    primitive_polygon_and_polyline_arrow_shapes;
extern const std::unordered_set<std::string_view> all_primitive_arrow_shapes;
extern const std::unordered_set<std::string_view>
    primitive_arrow_shapes_without_closed_svg_shape;

/// arrow shape modifiers
extern const std::unordered_set<std::string_view> all_arrow_shape_modifiers;

/// rank directions
extern const std::unordered_set<std::string_view> all_rank_directions;

/// edge directions
extern const std::unordered_set<std::string_view> all_edge_directions;

/// misc utilities

/// get the base name of the test case file without file extension
#define AUTO_NAME() std::filesystem::path(__FILE__).stem().string()

void write_to_file(const std::filesystem::path &directory,
                   const std::filesystem::path &filename,
                   std::string_view text);
