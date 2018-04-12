// std
#include <iterator> // std::back_inserter
#include <string>
#include <regex>
#include <vector>
// AmazingCow Libs
#include "acow/algo.h"
#include "acow/cpp_goodies.h"
#include "CMD/CMD.h"
#include "CoreDir/CoreDir.h"
#include "CoreFile/CoreFile.h"
#include "CoreFS/CoreFS.h"
#include "CoreLog/CoreLog.h"
// COWTODO
#include "Logger.h"
#include "ExcludeDirsRC.h"

//----------------------------------------------------------------------------//
// Constants                                                                  //
//----------------------------------------------------------------------------//
constexpr auto kLF   = "\n";
constexpr auto kCRLF = "\r\n";


//----------------------------------------------------------------------------//
// Globals                                                                    //
//----------------------------------------------------------------------------//
acow_global_variable std::vector<std::string> g_AllowedExtensions;



//----------------------------------------------------------------------------//
// Types                                                                      //
//----------------------------------------------------------------------------//
struct FileInfo 
{
    bool is_empty = true;
};

struct ScanResult
{
    std::vector<FileInfo> file_infos;
};


//----------------------------------------------------------------------------//
// Helper Functions                                                           //
//----------------------------------------------------------------------------//
acow_internal_function void
InitGlobals()
{
    g_AllowedExtensions.swap(std::vector<std::string>({"cpp", "h"}));
}

acow_internal_function void
PrintAndExit(const std::string &str) noexcept 
{
    printf("%s\n", str.c_str());
    exit(0);
}

acow_internal_function std::string 
GetVersionString() noexcept
{
    return "Version: ";
}
acow_internal_function std::string
GetExcludeDirsRC() noexcept
{
    ExcludeDirsRC::Initialize();
    const auto &excluded_dirs = ExcludeDirsRC::GetExcludedPaths();
    auto result               = CoreString::Join(CoreFS::NewLine(), excluded_dirs);

    return result;
}

//----------------------------------------------------------------------------//
// Sanitize Functions                                                         //
//----------------------------------------------------------------------------//
acow_internal_function std::vector<std::string>
SanitizeExcludeDirsRC(const std::vector<std::string> &paths) noexcept
{
    auto sanitized_paths = std::vector<std::string>();
    sanitized_paths.reserve(paths.size());

    for(const auto &path : paths) { 
        const auto abs_path = CoreFS::ExpandUserAndMakeAbs(path);
        if(CoreFS::IsDir(abs_path)) { 
            sanitized_paths.push_back(abs_path);
        } else { 
            GetLogger()->Error("Invalid Path: (%s) - Ignoring it...", path);
        }
    }
    
    acow::algo::sort_and_unique(sanitized_paths);
    return sanitized_paths;
}

acow_internal_function std::vector<std::string>
SanitizeExcludeExtensions(const std::vector<std::string> &exts) noexcept
{
    auto sanitized_exts = std::vector<std::string>();
    sanitized_exts.reserve(exts.size());

    for(const auto ext : exts) { 
        auto clean_ext = CoreString::TrimStart(".");
        if(acow::algo::contains(g_AllowedExtensions, clean_ext)) { 
            sanitized_exts.push_back(clean_ext);
        } else { 
            GetLogger()->Error("Invalid Extension: (%s) - Ignoring it...", ext);
        }
    }

    acow::algo::sort_and_unique(sanitized_exts);
    return sanitized_exts;
}

//----------------------------------------------------------------------------//
// Build Functions                                                            //
//----------------------------------------------------------------------------//
acow_internal_function std::vector<std::string> 
BuildUsableExtensions(const std::vector<std::string> &excludedExts) noexcept 
{
    auto usable_extensions = std::vector<std::string>();
    usable_extensions.reserve(g_AllowedExtensions.size());
 
    std::set_difference(
        g_AllowedExtensions.begin(), 
        g_AllowedExtensions.end  (), 
        excludedExts.begin(), 
        excludedExts.end  (), 
        std::back_inserter(usable_extensions)
    );

    return usable_extensions;
}

acow_internal_function std::string 
BuildFindExtensionsRegexString(
    const std::vector<std::string> &excludedExts) noexcept 
{
    constexpr auto kExtPart_Fmt = R"(.*\.%s)";    
    const auto &usable_extensions = BuildUsableExtensions(excludedExts);
    
    // COWHACK(n2omatt): Very ineficienty... but quick to develop...
    std::stringstream ss;

    const auto exts_size = usable_extensions.size();
    for(size_t i = 0; i < exts_size ; ++i){ 
        ss << CoreString::Format(kExtPart_Fmt, usable_extensions[i]);
        if(i != exts_size -1) {
            ss << "|";
        }
    }

    return CoreString::Format("(%s)", ss.str());
}

//----------------------------------------------------------------------------//
// Checker Functions                                                          //
//----------------------------------------------------------------------------//
acow_internal_function inline bool
CheckLineIsEmpty(const std::string &line) noexcept
{
    return line.empty() || line == kLF || line == kCRLF;
}

//----------------------------------------------------------------------------//
// Find Functions                                                             //
//----------------------------------------------------------------------------//
acow_internal_function std::vector<std::string>
FindCommentLines(
    const std::vector<std::string> &fileLines, 
    size_t                          startLineNo,
    size_t                          fileLinesSize,
    const std::string              &fileExtension) noexcept
{
}

acow_internal_function bool
CheckLineHasEndComment(const std::string &line, const std::string &file_ext)
{
    return false;
}


//----------------------------------------------------------------------------//
// Output Functions                                                           //
//----------------------------------------------------------------------------//
acow_internal_function void
OutputResultsLong(const ScanResult &results, bool no_colors_requested) noexcept
{
}

acow_internal_function void
OutputResultsShort(const ScanResult &results, bool no_colors_requested) noexcept
{
}


//----------------------------------------------------------------------------//
// Scan / Parse Functions                                                     //
//----------------------------------------------------------------------------//
acow_internal_function FileInfo
Parse(const std::string &filename) noexcept
{
    FileInfo file_info;
    
    const auto file_lines = CoreFile::ReadAllLines(filename);
    if(file_lines.empty()) { 
        return file_info;
    }
    
    const auto file_ext   = CoreFS::SplitExt(filename).second;
    const auto lines_size = file_lines.size();
    for(size_t line_no = 0; line_no < lines_size; ++line_no) { 
        const auto &line = file_lines[line_no];
        // Check if is empty...
        if(CheckLineIsEmpty(line)) { 
            continue;
        }

        // Grab all lines that makes a comment block.
        auto commented_lines = FindCommentedLines(
            file_lines, 
            line_no, 
            lines_size, 
            file_ext
        );
                

        // Make the line_no beware or the consumed lines.
        line_no += commented_lines.size();
    }

    return file_info;
}

acow_internal_function ScanResult 
Scan(
    const std::string              &startPath, 
    const std::vector<std::string> &excludedDirs, 
    const std::vector<std::string> &excludedExts) noexcept
{
    //--------------------------------------------------------------------------
    // Check if the start path is valid.
    auto start_fullpath = CoreFS::ExpandUserAndMakeAbs(startPath);
    if(!CoreFS::IsDir(start_fullpath)) { 
        GetLogger()->Fatal("Invalid start path: (%s)", start_fullpath);
    }
    
    ScanResult scan_result;

    //--------------------------------------------------------------------------
    // Clean the excluded directories.    
    auto directories       = CoreDir::GetDirectories(start_fullpath, ".*", true);
    directories.push_back(start_fullpath); // COWTODO(n2omatt): Include the give dir???

    auto clean_directories = directories;
    // COWHACK(n2omatt): Very ineficienty... but quick to develop...
    for(const auto &directory : directories) { 
        acow::algo::remove_if(
            clean_directories, 
            [&directory, &excludedDirs](const std::string &path) {                 
                for(auto excluded_dir : excludedDirs) {
                    return CoreString::StartsWith(path, excluded_dir);
                }
                return false;
            },
            true
        );
    }

    if(clean_directories.empty()) { return scan_result; }

    //--------------------------------------------------------------------------
    // Check each file found.    
    const auto regex_str = BuildFindExtensionsRegexString(excludedExts);
    for(const auto &directory : clean_directories) { 
        auto files = CoreDir::GetFiles(directory, regex_str, false);
        for(const auto &file : files) {
            auto file_info = Parse(file);
            if(!file_info.is_empty) { 
                scan_result.file_infos.push_back(file_info);
            } else { 
                continue;
            }
        } 
    }
    
    return scan_result;
}


int 
main(int argc, char *argv[]) 
{
    //--------------------------------------------------------------------------
    // Initialize.
    InitGlobals();

    //--------------------------------------------------------------------------
    // Create the Flags.
    auto flag_help = CMD::Flag::Create(
        "h", "help", "Display this screen.", 
        CMD::NoArgument | CMD::StopOnView
    );

    auto flag_version = CMD::Flag::Create(
        "v", "version", "Display version information.", 
        CMD::NoArgument | CMD::StopOnView
    );

    auto flag_verbose = CMD::Flag::Create(
        "V", "verbose", "Show extra output.", 
        CMD::NoArgument
    );

    auto flag_no_colors= CMD::Flag::Create(
        "", "no-colors", "Make the output uncolored.", 
        CMD::NoArgument
    );

    auto flag_list_exlude_dir = CMD::Flag::Create(
        "", "list-exclude-dir", "Display the saved excluded directories.",
        CMD::NoArgument | CMD::StopOnView
    );
    
    auto flag_long = CMD::Flag::Create(
        "l", "long", "Display long output.", 
        CMD::NoArgument
    );

    auto flag_short = CMD::Flag::Create(
        "s", "short", "Display short output.",
        CMD::NoArgument
    );

    auto flag_exclude_dirs = CMD::Flag::Create(
        "e", "exclude-dir", "Exclude the directory on this run.", 
        CMD::RequiredArgument | CMD::AllowDuplicates | CMD::MergeArgument
    );

    auto flag_exclude_exts = CMD::Flag::Create(
        "E", "exclude-ext", "Exclude the extension on this run.", 
        CMD::RequiredArgument | CMD::AllowDuplicates | CMD::MergeArgument
    );

    auto flag_add_exclude_dir = CMD::Flag::Create(
        "", "add-exclude-dir", "Add the directory to the saved excluded list.", 
        CMD::RequiredArgument | CMD::AllowDuplicates | CMD::MergeArgument
    );

    auto flag_remove_exclude_dir = CMD::Flag::Create(
        "", "remove-exclude-dir", "Remove the directory from the saved excluded list.", 
        CMD::RequiredArgument | CMD::AllowDuplicates | CMD::MergeArgument
    );

    //--------------------------------------------------------------------------
    // Parse the command line args.
    auto parser = CMD::Parser(CMD::DieOnNonValidFlags, { 
        flag_help,
        flag_version,
        flag_verbose,
        flag_no_colors,
        flag_list_exlude_dir,
        flag_long,
        flag_short,
        flag_exclude_dirs,
        flag_exclude_exts,
        flag_add_exclude_dir,
        flag_remove_exclude_dir
    });
    parser.Parse(argc, argv);

    //--------------------------------------------------------------------------
    // Options switches.
    auto long_requested         = false;
    auto short_requested        = false;
    auto verbose_requested      = false;
    auto no_colors_requested    = false;
    auto exclude_dirs           = std::vector<std::string>();
    auto exclude_exts           = std::vector<std::string>();
    auto exclude_dirs_to_add    = std::vector<std::string>();
    auto exclude_dirs_to_remove = std::vector<std::string>();

    //--------------------------------------------------------------------------
    // Get Flags Values.    
    // Verbose / No Colors - Optionals.
    if(flag_verbose  ->Found()) verbose_requested   = true;
    if(flag_no_colors->Found()) no_colors_requested = true;
    
    //--------------------------------------------------------------------------
    // Configure the Logger.
    //   We need configure it here, because SanitizeExcluded* and other functions 
    //   may find errors and it will output stuff based on this information.
    GetLogger()->SetLevel(
        (verbose_requested) 
            ? CoreLog::Logger::LOG_LEVEL_VERBOSE
            : CoreLog::Logger::LOG_LEVEL_NONE
    );
    GetLogger()->SetColor(
        (no_colors_requested)
            ? CoreLog::Logger::LOG_COLOR_DISABLE
            : CoreLog::Logger::LOG_COLOR_ENABLE
    );

    // Help / Version / List exclude dirs - Exclusives.
    if(flag_help           ->Found()) PrintAndExit(parser.GenerateHelpString()); 
    if(flag_version        ->Found()) PrintAndExit(GetVersionString         ());
    if(flag_list_exlude_dir->Found()) PrintAndExit(GetExcludeDirsRC         ());
    
    // Long / Short output - Optionals.
    if(flag_long ->Found()) long_requested  = true;
    if(flag_short->Found()) short_requested = true;

    // Exclude dir - Optional
    if(flag_exclude_dirs->Found()) { 
        exclude_dirs = SanitizeExcludeDirsRC(flag_exclude_dirs->GetValues());
    }

    // Exclude extension - Optional
    if(flag_exclude_exts->Found()) { 
        exclude_exts = SanitizeExcludeExtensions(flag_exclude_exts->GetValues());
    }

    // Add / Remove the dir to exclude rc - Optionals
    if(flag_add_exclude_dir->Found()) { 
        exclude_dirs_to_add = SanitizeExcludeDirsRC(flag_add_exclude_dir->GetValues());
    }
    if(flag_remove_exclude_dir->Found()) { 
        exclude_dirs_to_remove = SanitizeExcludeDirsRC(flag_remove_exclude_dir->GetValues());
    }

    //--------------------------------------------------------------------------
    // Add/Remove all paths to/from rc before start the run.
    ExcludeDirsRC::Initialize();
        ExcludeDirsRC::AddPaths   (exclude_dirs_to_add   );
        ExcludeDirsRC::RemovePaths(exclude_dirs_to_remove);
    ExcludeDirsRC::Save();
    
    //--------------------------------------------------------------------------
    // Add all the Excluded paths in rc to the list of excluded paths.
    acow::algo::copy_insert(
        ExcludeDirsRC::GetExcludedPaths(),
        exclude_dirs
    );
    
    //--------------------------------------------------------------------------
    // Get the start path.
    std::string start_path;
    if(!parser.GetNonFlagValues().empty()) {
        start_path = parser.GetNonFlagValues().front();
    } else {
        start_path = "C:/Users/n2omatt/Documents/Projects/AmazingCow/AmazingCow-Tools/crlf2lf/src"; // Current directory.
    }

    //--------------------------------------------------------------------------
    // Run.
    auto results = Scan(start_path, exclude_dirs, exclude_exts);
    if(long_requested) { 
        OutputResultsLong(results, no_colors_requested); 
    } else { 
        OutputResultsShort(results, no_colors_requested);
    }
}