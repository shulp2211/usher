# Ultrafast Sample Placement on Existing Trees (UShER)

[license-badge]: https://img.shields.io/badge/License-MIT-yellow.svg 
[license-link]: https://github.com/yatisht/usher/blob/master/LICENSE

[![License][license-badge]][license-link]
[![Build Status](https://github.com/yatisht/usher/workflows/build/badge.svg)](https://github.com/yatisht/usher/actions)

<img src="/images/usher_logo.png" width="600">

UShER is a program that rapidly places new samples onto an existing phylogeny using maximum parsimony. It is particularly helpful in understanding the relationships of newly sequenced SARS-CoV-2 genomes with each other and with previously sequenced genomes in a global phylogeny. This has emerged as an important challenge during this pandemic for enabling *genomic contact tracing* since the viral phylogeny is already very large (>50K sequences, see https://github.com/roblanf/sarscov2phylo/releases) and is expected to grow by many fold in the coming months. 

UShER is much faster than existing tools with similar functionality and has now also been integrated in the [UCSC SARS-CoV-2 Genome Browser](https://genome.ucsc.edu/cgi-bin/hgPhyloPlace).

## Installing UShER

First, clone the UShER repository using the commands below.
```
git clone https://github.com/yatisht/usher.git
cd usher
```

Next, you may install UShER either using [Docker](https://www.docker.com/) (**recommended**) or using one of the installation scripts that we provide for **MacOS (10.14 and above)**, **Ubuntu (18.04 and above)** and **CentOS (7 and above)**. See instructions below.

### Using Docker

Perhaps the simplest method to install this package irrespective of the platform (Windows, Linux or MacOS) is using [Docker](https://www.docker.com/) (see installation instructions [here](https://docs.docker.com/get-docker/)). Once Docker is installed (which takes a few minutes typically), use the following command to build and install UShER.

```
docker build -t usher .
```
Once installed, you may start a new bash in a Ubuntu 18.04 container using the Docker command below, following which the remaining commands on this page should work smoothly.
```
docker run -t -i usher /bin/bash
```

### Using installation scripts

If you don't have Docker or prefer not to use it for some reason, you may use one of our installation scripts below to install UShER.

* For MacOS 10.14 and above: 
```
./installMacOS.sh  
```
* For Ubuntu 18.04 and above (requires sudo privileges):
```
./installUbuntu.sh  
```
* For CentOS 7 and above (requires sudo privileges): 
```
./installCentOS.sh  
```

## How UShER works?

Given existing samples, whose genotypes and phylogenetic tree is known, and the genotypes of new samples, UShER aims to incorporate new samples into the phylogenetic tree while preserving the topology of existing samples and maximizing parsimony. UShER’s algorithm consists of two phases: (i) the pre-processing phase and (ii) the placement phase.

In the **pre-processing phase**, UShER accepts the phylogenetic tree of existing samples in a Newick format and their genotypes, specified as a set of single-nucleotide variants with respect to a reference sequence (UShER currently ignores indels), in a VCF format. For each site in the VCF, UShER uses [Fitch-Sankoff algorithm](https://evolution.gs.washington.edu/gs541/2010/lecture1.pdf) to find the most parsimonious nucleotide assignment for every node of the tree. When a sample contains **ambiguous genotypes**, multiple nucleotides may be most parsimonious at a node. To resolve these, UShER assigns it any one of the most parsimonious nucleotides with preference, when possible, given to the reference base. UShER also allows the VCF to specify ambiguous bases in samples using [IUPAC format](https://www.bioinformatics.org/sms/iupac.html), which are also resolved to a unique base using the above strategy. When a node is found to carry a mutation, i.e. the base assigned to the node differs from its parent, the mutation gets added to a list of mutations corresponding to that node. Finally, UShER uses [protocol buffers](https://developers.google.com/protocol-buffers) to store in a file, the Newick string corresponding to the input tree and a list of lists of node mutation, which we refer to as **mutation-annotated tree object**, as shown in the figure below.

![pre-processing](/images/pre-processing.png)

The mutation-annotated tree object carries sufficient information to derive parsimony-resolved genotypes for any tip of the tree using the sequence of mutations from the root to that tip. For example, in the above figure, S5 can be inferred to contain variants G1149U and A2869G with respect to the reference sequence. Compared to other tools that use full multiple-sequence alignment (MSA) to guide the placement, UShER's mutation-annotated tree object is compact and is what helps make it **fast**.

In the **placement phase**, UShER loads the pre-processed mutation-annotated tree object and the genotypes of new samples in a VCF format and **sequentially** adds the new samples to the tree. For each new sample, UShER computes the additional parsimony score required for placing it at every node in the current tree while considering the full path of mutations from the root of the tree to that node. Next, UShER places the new sample at the node that results in the smallest additional parsimony score. When multiple node placements are equally parsimonious, UShER picks the node with a greater number of descendant leaves for placement. If the choice is between a parent and its child node, the parent node would always be selected by this rule. However, a more accurate placement should reflect the number of leaves uniquely attributable to the child versus parent node. Therefore, in these cases, UShER picks the parent node if the number of descendant leaves of the parent that are not shared with the child node exceed the number of descendant leaves of the child. The figure below shows a new sample, S7, containing variants G1149U and C9977A being added to the previous mutation-annotated tree object in a parsimony-optimal fashion (with a parsimony score of 1 for the mutation C9977A). UShER also automatically imputes and reports **ambiguous genotypes** for the newly added samples and ignores **missing bases**, such as 'N' or '.' (i.e. missing bases never contribute to the parsimony score).

![placement](/images/placement.png)

At the end of the placement phase, UShER allows the user to create another protocol-buffer file containing the mutation-annotated tree object for the newly generated tree including added samples as also shown in the example figure above. This allows for another round of placements to be carried out over and above the newly added samples. 

## Using UShER

### Pre-processing global phylogeny

The following example command pre-processes the existing phylogeny (global_phylo.nh) and using the genotypes (global_samples.vcf) and generates the mutation-annotated tree object that gets stored in a protobuf file (global_assignments.pb). 
```
./build/usher --tree test/global_phylo.nh --vcf test/global_samples.vcf --save-assignments global_assignments.pb 
```
By default, UShER uses **all available threads** but the user can also specify the number of threads using the *--threads* command-line parameter.

UShER also allows an option during the pre-processing phase to collapse nodes (i.e. delete the node after moving its child nodes to its parent node) that are not inferred to contain a mutation through the Fitch-Sankoff algorithm as well as to condense nodes that contain identical sequences into a single representative node. This is the **recommended usage** for UShER as it not only helps in significantly reducing the search space for the placement phase but also helps reduce ambiguities in the placement step and can be done by setting the *--collapse-final-tree* parameter.  
```
./build/usher --tree test/global_phylo.nh --vcf test/global_samples.vcf --save-assignments global_assignments.pb --collapse-final-tree
```

Note the the above command would condense identical sequences, namely S2, S3 and S4, in the example figure above into a single condensed new node (named something like *node_1_condensed_3_leaves*). If you wish to display the collapsed tree without condensing the nodes, also set the *--print-uncondensed-final-tree* option, for example, as follows:
```
./build/usher --tree test/global_phylo.nh --vcf test/global_samples.vcf --save-assignments global_assignments.pb --collapse-final-tree --print-uncondensed-final-tree
```

Also note that the placement phase in UShER would automatically place onto the input global phylogeny any samples in the VCF that missing in the input global phylogeny using its parsimony-optimal placement algorithm.

### Placing new samples

Once the pre-processing is complete and a mutation-annotated tree object is generate (e.g. global_assignments.pb), UShER can place new sequences whose variants are called in a VCF file (e.g. missing_samples.vcf) to existing tree as follows:

```
./build/usher --load-assignments pglobal_assignments.pb --vcf test/missing_samples.vcf --print-uncondensed-final-tree
```
Again, by default, UShER uses **all available threads** but the user can also specify the number of threads using the *--threads* command-line parameter.

The above command not only places each new sample sequentially, but also reports the parsimony score and the number of parsimony-optimal placements found for each added sample.

In addition to the global phylogeny, one often needs to contextualize the newly added sequences using subtrees of closest *N* neighbouring sequences, where *N* is small. UShER allows this functionality using *--print-subtrees-size*, which can be set to an arbitrary *N*, such as 20 in the example below:

```
./build/usher --load-assignments pglobal_assignments.pb --vcf test/missing_samples.vcf --print-uncondensed-final-tree --print-subtrees-size 20
```

### Uncertainty in placing new samples


## Reference
* Coming soon