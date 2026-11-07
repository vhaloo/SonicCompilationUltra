#include "voxel_export.h"
#include <array>
#include <fstream>
#include <iomanip>
#include <locale>
#include <stdexcept>
#include <string>
namespace scu::voxel {
void exportScene(const Frame &frame, const Mesher &mesh, const std::filesystem::path &directory) {
    if (!frame.valid() || !mesh.valid())
        throw std::runtime_error("No complete pixel-volume scene to export");
    if (std::filesystem::exists(directory))
        throw std::runtime_error("Keep the existing export; choose a new folder");
    if (!std::filesystem::create_directories(directory))
        throw std::runtime_error("Cannot create the export folder");
    std::ofstream texture(directory / "pixels.tga", std::ios::binary),
        material(directory / "scene.mtl"), object(directory / "scene.obj"),
        readme(directory / "READ_ME.txt");
    if (!texture || !material || !object || !readme)
        throw std::runtime_error("Cannot write the export files");
    const int width = frame.textureWidth(), height = frame.textureHeight();
    // Uncompressed 32-bit TGA, top-left origin, eight alpha bits. Explicit
    // channel writes make the file independent of host endianness.
    std::array<unsigned char, 18> header{};
    header[2] = 2;
    header[12] = width & 255;
    header[13] = (width >> 8) & 255;
    header[14] = height & 255;
    header[15] = (height >> 8) & 255;
    header[16] = 32;
    header[17] = 0x28;
    texture.write(reinterpret_cast<const char *>(header.data()), header.size());
    for (auto pixel : frame.pixels)
        for (unsigned shift : {0u, 8u, 16u, 24u})
            texture.put(char(pixel >> shift));
    material
        << "# Local pixel-volume export. Keep cartridge-derived artwork private.\n"
           "newmtl opaque\nKa 0.4 0.4 0.4\nKd 1 1 1\nKs 0 0 0\nd 1\nillum 1\nmap_Kd pixels.tga\n\n"
           "newmtl translucent\nKa 0.4 0.4 0.4\nKd 1 1 1\nKs 0 0 0\nd 1\nillum 1\n"
           "map_Kd pixels.tga\nmap_d -imfchan a pixels.tga\n";
    object.imbue(std::locale::classic());
    object << std::setprecision(9);
    object << "# Sonic Compilation Ultra local procedural pixel-volume scene\n"
              "# Scale: 1 original world pixel = 0.01 model units. Y points up.\n"
              "mtllib scene.mtl\ns off\n";
    const std::array<const char *, planeCount> names{{"background", "terrain_low", "actors_low",
                                                      "background_high", "terrain_high",
                                                      "actors_high", "spawn_previews"}};
    const float cx = float(frame.left + frame.coverageWidth / 2),
                cy = float(frame.top + frame.coverageHeight / 2), cz = mesh.bounds().min.z;
    size_t first = 1;
    for (size_t plane = 0; plane < planeCount; ++plane)
        for (int alpha = 0; alpha < 2; ++alpha) {
            const auto &vertices =
                alpha ? mesh.layers()[plane].translucent : mesh.layers()[plane].opaque;
            if (vertices.empty())
                continue;
            object << "g " << names[plane] << (alpha ? "_translucent" : "_opaque") << "\nusemtl "
                   << (alpha ? "translucent" : "opaque") << '\n';
            for (const auto &v : vertices)
                object << "v " << (v.position.x - cx) * .01f << ' ' << (cy - v.position.y) * .01f
                       << ' ' << (v.position.z - cz) * .01f << '\n';
            for (const auto &v : vertices)
                object << "vt " << v.u << ' ' << 1 - v.v << '\n';
            for (const auto &v : vertices)
                object << "vn " << v.normal.x << ' ' << -v.normal.y << ' ' << v.normal.z << '\n';
            for (size_t i = 0; i < vertices.size(); i += 3) {
                object << "f";
                // Y inversion changes handedness; normals and winding stay outward.
                for (size_t n : {i, i + 2, i + 1}) {
                    size_t index = first + n;
                    object << ' ' << index << '/' << index << '/' << index;
                }
                object << '\n';
            }
            first += vertices.size();
        }
    readme << "PRIVATE LOCAL PIXEL-VOLUME EXPORT\n\n"
              "Open scene.obj with its scene.mtl and pixels.tga in the same folder.\n"
              "Groups preserve observed terrain/actor priority and transparent previews.\n"
              "Pixels are procedural columns, not artist-authored hidden object models.\n"
              "Scale or move the whole collection in your modelling application.\n"
              "Material alpha import differs between applications; use the TGA alpha channel.\n"
              "Exports can contain artwork from your cartridge. Keep them private;\n"
              "public Sonic Compilation Ultra packages contain no cartridge artwork.\n";
    texture.flush();
    material.flush();
    object.flush();
    readme.flush();
    if (!texture || !material || !object || !readme)
        throw std::runtime_error("The export could not be written completely");
}
} // namespace scu::voxel
