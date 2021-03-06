/* Copyright 2016 Ramakrishnan Kannan */
#ifndef MPI_MPICOMM_HPP_
#define MPI_MPICOMM_HPP_

#include <mpi.h>
#include <vector>
#include "distutils.hpp"
#include "stacktrace.h"

class MPICommunicator {
  private:
    int m_rank;
    int m_numProcs;
    int m_row_rank, m_row_size;
    int m_col_rank, m_col_size;
    int m_pr, m_pc;

    // for 2D communicators
    // MPI Related stuffs
    MPI_Comm* m_commSubs;
    void printConfig() {
        if (rank() == 0) {
            INFO << "successfully setup MPI communicators" << endl;
            INFO << "size=" << size() << endl;
            INFO << "rowsize=" << m_row_size << ":pr=" << m_pr << endl;
            INFO << "colsize=" << m_col_size << ":pc=" << m_pc << endl;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        cout << ":rank=" << rank() << ":row_rank=" <<
             row_rank() << ":colrank" << col_rank() << endl;
    }

  public:
    // Violating the cpp guidlines. Other functions need
    // non const pointers.
    MPICommunicator(int argc, char *argv[]) {
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &m_numProcs);
    }
    ~MPICommunicator() {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
    }
    MPICommunicator(int argc, char *argv[], int pr, int pc) {
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &m_numProcs);
        int reorder = 0;
        std::vector<int> dimSizes;
        std::vector<int> periods;
        int nd = 2;
        dimSizes.resize(nd);
        this->m_pr = pr;
        this->m_pc = pc;
        dimSizes[0] = pr;
        dimSizes[1] = pc;
        periods.resize(nd);
        MPI_Comm gridComm;
        std::vector<int> gridCoords;
        fillVector<int>(1, &periods);
        // int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[],
        //                const int periods[], int reorder, MPI_Comm *comm_cart)
        if (dimSizes[0]*dimSizes[1] != m_numProcs) {
            if (m_rank == 0) {
                std::cerr << "Processor grid dimensions do not"
                          << "multiply to MPI_SIZE" << std::endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        MPI_Cart_create(MPI_COMM_WORLD, nd, &dimSizes[0], &periods[0],
                        reorder, &gridComm);
        gridCoords.resize(nd);
        MPI_Cart_get(gridComm, nd, &dimSizes[0], &periods[0], &(gridCoords[0]));
        this->m_commSubs = new MPI_Comm[nd];
        int* keepCols = new int[nd];
        for (int i = 0; i < nd; i++) {
            std::fill_n(keepCols, nd, 0);
            keepCols[i] = 1;
            MPI_Cart_sub(gridComm, keepCols, &(this->m_commSubs[i]));
        }
        MPI_Comm_size(m_commSubs[0], &m_row_size);
        MPI_Comm_size(m_commSubs[1], &m_col_size);
        MPI_Comm_rank(m_commSubs[0], &m_row_rank);
        MPI_Comm_rank(m_commSubs[1], &m_col_rank);
#ifdef MPI_VERBOSE
        printConfig();
#endif
    }
    const int rank() const {return m_rank;}
    const int size() const {return m_numProcs;}
    const int row_rank() const {return m_row_rank;}
    const int col_rank() const {return m_col_rank;}
    const int pr() const {return m_pr;}
    const int pc() const {return m_pc;}
    const MPI_Comm* commSubs() const {return m_commSubs;}
};
#endif  // MPI_MPICOMM_HPP_
