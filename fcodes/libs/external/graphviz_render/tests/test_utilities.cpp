#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

#include <fmt/format.h>

#include "test_utilities.h"

const std::unordered_set<std::string_view> node_shapes_consisting_of_ellipse = {
    "ellipse",      //
    "oval",         //
    "circle",       //
    "doublecircle", //
    "point",        //
};

const std::unordered_set<std::string_view>
    node_shapes_consisting_of_ellipse_and_polyline = {
        "Mcircle", //
};

const std::unordered_set<std::string_view> node_shapes_consisting_of_path = {
    "cylinder", //
};

const std::unordered_set<std::string_view> node_shapes_consisting_of_polygon = {
    "box",           //
    "polygon",       //
    "egg",           //
    "triangle",      //
    "diamond",       //
    "trapezium",     //
    "parallelogram", //
    "house",         //
    "pentagon",      //
    "hexagon",       //
    "septagon",      //
    "octagon",       //
    "doubleoctagon", //
    "tripleoctagon", //
    "invtriangle",   //
    "invtrapezium",  //
    "invhouse",      //
    "rect",          //
    "rectangle",     //
    "square",        //
    "star",          //
    "cds",           //
    "rpromoter",     //
    "rarrow",        //
    "larrow",        //
    "lpromoter",     //
    "folder",        //
};

const std::unordered_set<std::string_view>
    node_shapes_consisting_of_polygon_and_polyline = {
        "Mdiamond",        //
        "Msquare",         //
        "underline",       //
        "note",            //
        "tab",             //
        "box3d",           //
        "component",       //
        "promoter",        //
        "terminator",      //
        "utr",             //
        "primersite",      //
        "restrictionsite", //
        "fivepoverhang",   //
        "threepoverhang",  //
        "noverhang",       //
        "assembly",        //
        "signature",       //
        "insulator",       //
        "ribosite",        //
        "rnastab",         //
        "proteasesite",    //
        "proteinstab",     //
};

const std::unordered_set<std::string_view> node_shapes_without_svg_shape = {
    "plaintext", //
    "plain",     //
    "none",      //
};

const std::unordered_set<std::string_view> all_node_shapes = {
    "box",             //
    "polygon",         //
    "ellipse",         //
    "oval",            //
    "circle",          //
    "point",           //
    "egg",             //
    "triangle",        //
    "plaintext",       //
    "plain",           //
    "diamond",         //
    "trapezium",       //
    "parallelogram",   //
    "house",           //
    "pentagon",        //
    "hexagon",         //
    "septagon",        //
    "octagon",         //
    "doublecircle",    //
    "doubleoctagon",   //
    "tripleoctagon",   //
    "invtriangle",     //
    "invtrapezium",    //
    "invhouse",        //
    "Mdiamond",        //
    "Msquare",         //
    "Mcircle",         //
    "rect",            //
    "rectangle",       //
    "square",          //
    "star",            //
    "none",            //
    "underline",       //
    "cylinder",        //
    "note",            //
    "tab",             //
    "folder",          //
    "box3d",           //
    "component",       //
    "promoter",        //
    "cds",             //
    "terminator",      //
    "utr",             //
    "primersite",      //
    "restrictionsite", //
    "fivepoverhang",   //
    "threepoverhang",  //
    "noverhang",       //
    "assembly",        //
    "signature",       //
    "insulator",       //
    "ribosite",        //
    "rnastab",         //
    "proteasesite",    //
    "proteinstab",     //
    "rpromoter",       //
    "rarrow",          //
    "larrow",          //
    "lpromoter"        //
};

static const std::unordered_set<std::string_view>
    node_shapes_containing_multiple_same_shapes_with_different_fill = {
        "cylinder",      // two paths
        "doublecircle",  // two ellipses
        "doubleoctagon", // two polygons
        "tripleoctagon", // three polygons
};

bool contains_polygon_shape(const std::string_view shape) {
  return node_shapes_consisting_of_polygon.contains(shape) ||
         node_shapes_consisting_of_polygon_and_polyline.contains(shape);
}

bool contains_polyline_shape(const std::string_view shape) {
  return node_shapes_consisting_of_ellipse_and_polyline.contains(shape) ||
         node_shapes_consisting_of_polygon_and_polyline.contains(shape);
}

bool contains_ellipse_shape(const std::string_view shape) {
  return node_shapes_consisting_of_ellipse.contains(shape) ||
         node_shapes_consisting_of_ellipse_and_polyline.contains(shape);
}

bool contains_multiple_shapes_with_different_fill(
    const std::string_view shape) {
  return node_shapes_containing_multiple_same_shapes_with_different_fill
             .contains(shape) ||
         contains_polyline_shape(shape);
}

bool is_polygon_shape(const std::string_view shape) {
  return node_shapes_consisting_of_polygon.contains(shape);
}

const std::unordered_set<std::string_view> primitive_polygon_arrow_shapes = {
    "crow", "diamond", "inv", "normal", "vee"};

const std::unordered_set<std::string_view>
    primitive_polygon_and_polyline_arrow_shapes = {"box", "tee"};

const std::unordered_set<std::string_view> all_primitive_arrow_shapes = {
    "box", "crow", "curve",  "diamond", "dot", "icurve",
    "inv", "none", "normal", "tee",     "vee"};

const std::unordered_set<std::string_view>
    primitive_arrow_shapes_without_closed_svg_shape = {"curve", "icurve",
                                                       "none"};

const std::unordered_set<std::string_view> all_arrow_shape_modifiers = {
    "", "o", "l", "r"};

const std::unordered_set<std::string_view> all_rank_directions = {"TB", "BT",
                                                                  "LR", "RL"};

const std::unordered_set<std::string_view> all_edge_directions = {
    "forward", "back", "both", "none"};

void write_to_file(const std::filesystem::path &directory,
                   const std::filesystem::path &filename,
                   const std::string_view text) {
  std::filesystem::create_directories(directory);
  const std::filesystem::path test_artifacts_path = directory / filename;
  std::ofstream outfile{test_artifacts_path};
  if (!outfile.is_open()) {
    throw std::runtime_error{fmt::format("Could not create output file \"{}\"",
                                         test_artifacts_path.native())};
  }
  outfile << text;
}
