#include "describe.hpp"

std::vector<std::string> mutation_paths(const MAT::Tree& T, std::vector<std::string> samples) {
    std::vector<std::string> mpaths;
    for (auto sample: samples) {
        std::string cpath = sample + ": ";
        auto root_to_sample = T.rsearch(sample, true);
        std::reverse(root_to_sample.begin(), root_to_sample.end());
        //fprintf(stdout, "%s: ", sample.c_str());
        for (auto n: root_to_sample) {
            for (size_t i=0; i<n->mutations.size(); i++) {
                cpath += n->mutations[i].get_string();
                //fprintf(stdout, "%s", n->mutations[i].get_string().c_str());
                if (i+1 < n->mutations.size()) {
                    cpath += ",";
                    //fprintf(stdout, ",");
                }
            }
            if (n != root_to_sample.back()) {
                cpath += " > ";
                //fprintf(stdout, " > ");
            }
        }
        mpaths.push_back(cpath);
        //fprintf(stdout, "\n");
    }
    return mpaths;
}

std::vector<std::string> clade_paths(MAT::Tree T, std::vector<std::string> clades) {
    //get the set of clade path strings for printing
    //similar to the above, construct a series of strings to be printed or redirected later on
    std::vector<std::string> clpaths;
    clpaths.push_back("clade\tspecific\tfrom_root");
    //do a breadth-first search
    //the first time a clade that is in clades is encountered, that's the root;
    //get the path of mutations to that root (rsearch), save the unique mutations + that path
    //unique mutations being ones that occurred in the clade root, and the path being all mutations from that root back to the tree root
    //then continue. if a clade has already been encountered in the breadth first, its
    //not clade root, and should be skipped.
    std::unordered_set<std::string> clades_seen;

    auto dfs = T.breadth_first_expansion();
    for (auto n: dfs) {
        std::string curpath = n->identifier + "\t";
        for (auto ann: n->clade_annotations) {
            if (ann != "") {
                //if its one of our target clades and it hasn't been seen...
                if (std::find(clades.begin(), clades.end(), ann) != clades.end() && clades_seen.find(ann) != clades_seen.end()){
                    //get its own mutations, if there are any
                    std::string unique = "";
                    for (size_t i=0; i<n->mutations.size(); i++) {
                        unique += n->mutations[i].get_string();
                        if (i+1 < n->mutations.size()) {
                            unique += ",";
                        }
                    }
                    curpath += unique + "\t";
                    //get the ancestral mutations back to the root
                    std::string root = "";
                    auto root_to_sample = T.rsearch(n->identifier, true);
                    for (auto n: root_to_sample) {
                        for (size_t i=0; i<n->mutations.size(); i++) {
                            root += n->mutations[i].get_string();
                            if (i+1 < n->mutations.size()) {
                                root += ",";
                            }
                        }
                        if (n != root_to_sample.back()) {
                            root += " > ";
                        }
                    }
                    //save values to the string, mark the clade as seen, and save the string
                    curpath += unique;
                    curpath += root;
                    clades_seen.insert(ann);
                    clpaths.push_back(curpath);
                }
            }
        }
    }
    return clpaths;
}