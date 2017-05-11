#include "pacoss-partitioner.h"
#include "pacoss-sparse-struct.h"

Pacoss_Int numParts;
Pacoss_Int numColParts = 1;
char *sparseStructFileName;
char *dimPartitionFileName;
char *nzPartitionFileName;
const char *nzPartitionMethod = "fine-random";
const char *dimPartitionMethod = "balance-comm";
const char *patohSettings = "default";
char dimPartitionFileNameDefault[1000]; 
char nzPartitionFileNameDefault[1000];
char *command;
bool oneBasedIdx = true;

void printUsage()
{
  printfe("Usage: ./pacoss -c command command-args\n");
  printfe("  command: partition | distribute | check-stats | convert-sparse-struct\n");
  printfe("  partition-args: -f sparseStructFileName -m nzPartitionMethod -n dimPartitionMethod -p numParts [-z"
      " nzPartitionFileName -d dimPartitionFileName -q numColParts -s patohSettings]\n");
  printfe("  distribute-args: -f sparseStructFileName [-z nzPartitionFileName -d dimPartitionFileName]\n");
  printfe("  check-stats-args: -f sparseStructFileName [-z nzPartitionFileName -d dimPartitionFileName]\n");
  printfe("  convert-sparse-struct-args: -f sparseStructFileName [--one-based-idx]\n");
  printfe("  nzPartitionMethod: \n");
  printfe("    fine-random(default): Fine-grain random partitioning of sparse structure nonzeros.\n");
  printfe("    fine-patoh: Fine-grain hypergraph partitioning using PaToH.\n");
  printfe("    checkerboard-random: Random checkerboard partitioning.\n");
  printfe("    checkerboard-patoh-1d: Checkerboard partitioning using PaToH for row partitioning, and"
      " random column partitioning (to avoid multi-constraint partitioning).\n");
  printfe("    checkerboard-patoh-2d: Checkerboard partitioning using PaToH for row partitioning, and"
      " multiconstraint PaToH for column partitioning.\n");
  printfe("  dimPartitionMethod: \n");
  printfe("    balance-comm(default): Try to balance the communication while keeping the total communication volume at"
      " minimum. \n");
  printfe("    balance-rows: Try to balance the communication volume while balancing the number of rows owned"
      " by parts. This option can increase the total communication volume to establish row balance. \n");
  printfe("  patohSettings: \n");
  printfe("    quality: Uses more time for partitioning for slightly better quality.\n");
  printfe("    default(default): Default settings providing a speed/quality tradeoff.\n");
  printfe("    speed: Faster partitioning with sacrifice in quality.\n");
}

void processArguments(int argc, char **argv)
{
  for (Pacoss_Int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0) {
      sparseStructFileName = argv[i + 1];
      printfe("sparseStructFileName: %s\n", sparseStructFileName);
      i++;
    } else if (strcmp(argv[i], "-m") == 0) {
      nzPartitionMethod = argv[i + 1];
      printfe("nzPartitionMethod: %s\n", argv[i + 1]);
      i++;
    } else if (strcmp(argv[i], "-n") == 0) {
      dimPartitionMethod = argv[i + 1];
      printfe("dimPartitionMethod: %s\n", argv[i + 1]);
      i++;
    } else if (strcmp(argv[i], "-p") == 0) {
      sscanfe(argv[i + 1], " %" PRIINT, &numParts);
      printfe("numParts: %" PRIINT "\n", numParts);
      i++;
    } else if (strcmp(argv[i], "-q") == 0) {
      sscanfe(argv[i + 1], " %" PRIINT, &numColParts);
      printfe("numColParts: %" PRIINT "\n", numColParts);
      i++;
    } else if (strcmp(argv[i], "-z") == 0) {
      nzPartitionFileName = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "-d") == 0) {
      dimPartitionFileName = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "-s") == 0) {
      patohSettings = argv[i + 1];
      printfe("patohSettings: %s\n", patohSettings);
      i++;
    } else if (strcmp(argv[i], "--one-based-idx") == 0) {
      oneBasedIdx = true;
      printfe("oneBasedIdx: true\n");
    } else if (strcmp(argv[i], "partition") == 0 || strcmp(argv[i], "distribute") == 0 || strcmp(argv[i], "check-stats")
        == 0 || strcmp(argv[i], "convert-sparse-struct") == 0) {
      command = argv[i];
      printfe("command: %s\n", argv[i]);
    } else {
      printfe("WARNING: Ignoring argument: %s\n", argv[i]);
    }
  }
  if (command == NULL) {
    throw Pacoss_Error("ERROR: No valid command is specified.");
  }
  // In case no nzPartitionFileName or dimPartitionFileName is provided, use the defaults
  strcpy(nzPartitionFileNameDefault, sparseStructFileName);
  strcat(nzPartitionFileNameDefault, PACOSS_PARTITIONER_NZ_PARTITION_SUFFIX);
  if (nzPartitionFileName == NULL)  { nzPartitionFileName = nzPartitionFileNameDefault; }
  printfe("nzPartitionFileName: %s\n", nzPartitionFileName);
  strcpy(dimPartitionFileNameDefault, sparseStructFileName);
  strcat(dimPartitionFileNameDefault, PACOSS_PARTITIONER_DIM_PARTITION_SUFFIX);
  if (dimPartitionFileName == NULL) { dimPartitionFileName = dimPartitionFileNameDefault; }
  printfe("dimPartitionFileName: %s\n", dimPartitionFileName);
  printfe("\n");
}

void partition() {
  printfe("Loading sparse struct...\n");
  Pacoss_SparseStruct<double> ss;
  auto epoch = timerTic();
  ss.load(sparseStructFileName);
  printfe("Loading the sparse struct took %lf seconds.\n", timerToc(epoch));
  printfe("Partitioning sparse struct...\n");
  Pacoss_IntVector nzPart;
  std::vector<Pacoss_IntVector> dimPart;
  epoch = timerTic();
  if (strcmp(nzPartitionMethod, "fine-random") == 0) {
    Pacoss_Partitioner<double, Pacoss_Int>::partitionNzFineRandom(ss, numParts, nzPart);
  } else if (strcmp(nzPartitionMethod, "checkerboard-random") == 0) {
    Pacoss_Partitioner<double, Pacoss_Int>::partitionNzCheckerboardRandom(ss, numParts / numColParts, numColParts, nzPart);
  }
#ifdef PACOSS_USE_PATOH
  else if (strcmp(nzPartitionMethod, "fine-patoh") == 0) {
    Pacoss_Partitioner<double, Pacoss_Int>::partitionNzFinePatoh(ss, numParts, patohSettings, nzPart);
  } else if (strcmp(nzPartitionMethod, "checkerboard-patoh-1d") == 0) {
    Pacoss_Partitioner<double, Pacoss_Int>::partitionNzCheckerboardPatoh(ss, numParts / numColParts, numColParts, patohSettings,
        "1d", nzPart);
  } else if (strcmp(nzPartitionMethod, "checkerboard-patoh-2d") == 0) {
    Pacoss_Partitioner<double, Pacoss_Int>::partitionNzCheckerboardPatoh(ss, numParts / numColParts, numColParts, patohSettings,
        "2d", nzPart);
  }
#endif
  else {
    char msg[1000];
    sprintfe(msg, "ERROR: Unsupported nzPartitionMethod %s.\n", nzPartitionMethod);
    throw Pacoss_Error(msg);
  }
  printfe("Partitioning nonzeros took %lf seconds\n", timerToc(epoch));
  printfe("Partitioning dimensions...\n");
  epoch = timerTic();
  if (strcmp(dimPartitionMethod, "balance-comm") == 0) {
    Pacoss_Partitioner<double, Pacoss_Int>::partitionDimsBalanced(ss, numParts, nzPart, dimPart);
  } else if (strcmp(dimPartitionMethod, "balance-rows") == 0) {
    Pacoss_Partitioner<double, Pacoss_Int>::partitionDimsBalanced(ss, numParts, nzPart, dimPart, 1.25);
  } else if (strcmp(dimPartitionMethod, "random") == 0) {
    Pacoss_Partitioner<double, Pacoss_Int>::partitionDimsRandom(ss, numParts, dimPart);
  } else {
    char msg[1000];
    sprintfe(msg, "ERROR: Unsupported dimPartitionMethod %s.\n", dimPartitionMethod);
    throw Pacoss_Error(msg);
  }
  printfe("Partitioning dimensions took %lf seconds.\n", timerToc(epoch));
  printfe("Saving nonzero and dimension partition files...\n");
  epoch = timerTic();
  Pacoss_Partitioner<double, Pacoss_Int>::saveNzPart(nzPart, nzPartitionFileName);
  Pacoss_Partitioner<double, Pacoss_Int>::saveDimPart(numParts, dimPart, dimPartitionFileName);
  printfe("Saving partition files took %lf seconds.\n", timerToc(epoch));
  Pacoss_Partitioner<double, Pacoss_Int>::printPartitionStats(ss, numParts, nzPart, dimPart);
}

void distribute() {
  Pacoss_SparseStruct<double> ss;
  printfe("Loading sparse struct...\n");
  ss.load(sparseStructFileName);
  Pacoss_IntVector nzPart;
  std::vector<Pacoss_IntVector> dimPart;
  printfe("Loading dimPartitionFile...\n");
  Pacoss_Partitioner<double, Pacoss_Int>::loadDimPart(numParts, dimPart, dimPartitionFileName);
  printfe("Loading nzPartitionFile...\n");
  Pacoss_Partitioner<double, Pacoss_Int>::loadNzPart(nzPart, nzPartitionFileName);
  printfe("Distributing sparse struct...\n");
  Pacoss_Partitioner<double, Pacoss_Int>::distributeSparseStruct(ss, numParts, nzPart, sparseStructFileName);
  printfe("Distributing dimension partitions...\n");
  Pacoss_Partitioner<double, Pacoss_Int>::distributeDimPartition(sparseStructFileName, numParts, dimPart, dimPartitionFileName);
}

void checkStats() {
  Pacoss_SparseStruct<double> ss;
  printfe("Loading sparse struct...\n");
  ss.load(sparseStructFileName);
  Pacoss_IntVector nzPart;
  std::vector<Pacoss_IntVector> dimPart;
  printfe("Loading dimPartitionFile...\n");
  Pacoss_Partitioner<double, Pacoss_Int>::loadDimPart(numParts, dimPart, dimPartitionFileName);
  printfe("Loading nzPartitionFile...\n");
  Pacoss_Partitioner<double, Pacoss_Int>::loadNzPart(nzPart, nzPartitionFileName);
  Pacoss_Partitioner<double, Pacoss_Int>::printPartitionStats(ss, numParts, nzPart, dimPart);
}

void convertSparseStruct(bool oneBased = true) {
  FILE *file = fopene(sparseStructFileName, "r");
  char line[1000];
  std::vector<Pacoss_IntVector> idx;
  std::vector<double> val;

  // Find the order of sparse struct
  fgets(line, 1000, file);
  char *token = strtok(line, " ");
  Pacoss_Int order = -1; // Last entry is the value of the nonzero in each line
  while (token != NULL) {
    order++;
    token = strtok(NULL, " ");
  }
  printfe("order is %d\n", order);

  // Now form idx and val arrays from the file
  idx.resize(order);
  fclose(file); file = fopene(sparseStructFileName, "r");
  while (!feof(file)) {
    for (Pacoss_Int i = 0, ie = order; i < ie; i++) {
      Pacoss_Int temp;
      fscanf(file, " %" PRIINT, &temp);
      idx[i].push_back(temp);
    }
    double temp;
    fscanf(file, " %lf ", &temp);
    val.push_back(temp);
  }
  printfe("Read the sparse struct having %zu nonzero elements.\n", idx[0].size());
  Pacoss_SparseStruct<double>ss; ss.form(idx, val, oneBased);
  auto ssFileName = std::string(sparseStructFileName) + ".ss";
  ss.save(ssFileName.c_str());
  fclosee(file);
}

int main(int argc, char **argv)
{
  printfe("Pacoss v0.1\n\n");
  if (argc < 2)  {
    printUsage();
  } else {
    processArguments(argc, argv);
    if (strcmp(command, "partition") == 0) {
      partition();
    } else if (strcmp(command, "distribute") == 0) {
      distribute();
    } else if (strcmp(command, "check-stats") == 0) {
      checkStats();
    } else if (strcmp(command, "convert-sparse-struct") == 0) {
      convertSparseStruct();
    }
  }
  printfe("\nPacoss terminated..\n");
  return 0;
}
