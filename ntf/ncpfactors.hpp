/* Copyright 2017 Ramakrishnan Kannan */
#ifndef NTF_CPFACTORS_HPP_
#define NTF_CPFACTORS_HPP_

#include <cassert>
#include "utils.h"
#include "tensor.hpp"

// ncp_factors contains the factors of the ncp
// every ith factor is of size n_i * k
// number of factors is called as order of the tensor
// all idxs are zero idx.

namespace PLANC {

class NCPFactors {
    MAT *ncp_factors;
    int m_order;
    int m_k;
    UVEC m_dimensions;
    MAT lambda;

    //normalize the factors of a matrix

    void normalize(int order) {
        for (int i = 0; i < this->m_k; i++) {
            lambda(order, i) = arma::norm(this->ncp_factors[order].col(i));
            this->ncp_factors[order].col(i) /= lambda(order, i);
        }
    }

  public:

    //constructors
    NCPFactors(const UVEC i_dimensions, const int i_k, bool trans = false)
        : m_dimensions(i_dimensions) {
        this->m_order = i_dimensions.n_rows;
        ncp_factors = new MAT[this->m_order];
        this->m_k = i_k;
        UWORD numel = arma::prod(this->m_dimensions);
        for (int i = 0; i < this->m_order; i++) {
            // ncp_factors[i] = arma::randu<MAT>(i_dimensions[i], this->m_k);
            if (trans) {
                ncp_factors[i] = arma::randu<MAT>(this->m_k, i_dimensions[i]);

            } else {
                //ncp_factors[i] = arma::randi<MAT>(i_dimensions[i], this->m_k,
                //                                   arma::distr_param(0, numel));
                ncp_factors[i] = arma::randu<MAT>(i_dimensions[i], this->m_k);
            }
        }
        lambda = arma::ones<MAT>(this->m_order, this->m_k);
    }
    // getters
    int rank() const {return m_k;}
    UVEC dimensions() const {return m_dimensions;}
    MAT& factor(const int i_n) const {return ncp_factors[i_n];}

    // setters
    void set(const int i_n, const MAT &i_factor) {
        assert(i_factor.size() == this->ncp_factors[i_n].size());
        this->ncp_factors[i_n] = i_factor;
    }

    //compute gram of all local factors
    void gram(MAT *o_UtU) {
        MAT currentGram(this->m_k, this->m_k);
        for (int i = 0; i < this->m_order; i++) {
            currentGram = ncp_factors[i].t() * ncp_factors[i];
            (*o_UtU) = (*o_UtU) % currentGram;
        }
    }

    // find the hadamard product of all the factor grams
    // except the n. This is equation 50 of the JGO paper.
    void gram_leave_out_one(const int i_n, MAT *o_UtU) {
        MAT currentGram(this->m_k, this->m_k);
        (*o_UtU) = arma::ones<MAT>(this->m_k, this->m_k);
        for (int i = 0; i < this->m_order; i++) {
            if (i != i_n) {
                currentGram = ncp_factors[i].t() * ncp_factors[i];
                (*o_UtU) = (*o_UtU) % currentGram;
            }
        }
    }

    MAT krp_leave_out_one(const int i_n) {
        UWORD krpsize = arma::prod(this->m_dimensions);
        krpsize /= this->m_dimensions[i_n];
        MAT krp(krpsize, this->m_k);
        krp_leave_out_one(i_n, &krp);
        return krp;
    }
    // construct low rank tensor using the factors

    // khatrirao leaving out one. we are using the implementation
    // from tensor toolbox. Always krp for mttkrp is computed in
    // reverse. Hence assuming the same. The order of the computation
    // is same a tensor tool box.
    // size of krp must be product of all dimensions leaving out nxk
    void krp_leave_out_one(const int i_n, MAT *o_krp) {
        // matorder = length(A):-1:1;
        // Always krp for mttkrp is computed in
        // reverse. Hence assuming the same.
        UVEC matorder = arma::zeros<UVEC>(this->m_order - 1);
        int current_ncols = this->m_k;
        int j = 0;
        for (int i = this->m_order - 1; i >= 0; i--) {
            if (i != i_n ) {
                matorder(j++) = i;
            }
        }
#ifdef NTF_VERBOSE
        INFO << "::" << __PRETTY_FUNCTION__
             << "::" << __LINE__
             << "::matorder::" << matorder << std::endl;
#endif
        (*o_krp).zeros();
        // N = ncols(1);
        // This is our k. So keep N = k in our case.
        // P = A{matorder(1)};
        // take the first factor of matorder
        /*UWORD current_nrows = ncp_factors[matorder(0)].n_rows - 1;
        (*o_krp).rows(0, current_nrows) = ncp_factors[matorder(0)];
        // this is factor by factor
        for (int i = 1; i < this->m_order - 1; i++) {
            // remember always krp in reverse order.
            // That is if A krp B krp C, we compute as
            // C krp B krp A.
            // prev_nrows = current_nrows;
            // rightkrp.n_rows;
            // we are populating column by column
            MAT& rightkrp = ncp_factors[matorder[i]];
            for (int j = 0; j < this->m_k; j++) {
                VEC krpcol = (*o_krp)(arma::span(0, current_nrows), j);
                // krpcol.each_rows*rightkrp.col(i);
                for (int k = 0; k < rightkrp.n_rows; k++) {
                    (*o_krp)(arma::span(k * krpcol.n_rows, (k + 1)*krpcol.n_rows - 1), j) = krpcol * rightkrp(k, j);
                }
            }
            current_nrows *= rightkrp.n_rows;
        }*/
// Loop through all the columns
// for n = 1:N
//     % Loop through all the matrices
//     ab = A{matorder(1)}(:,n);
//     for i = matorder(2:end)
//        % Compute outer product of nth columns
//        ab = A{i}(:,n) * ab(:).';
//     end
//     % Fill nth column of P with reshaped result
//     P(:,n) = ab(:);
// end
        for (int n = 0; n < this->m_k; n++) {
            MAT ab = ncp_factors[matorder[0]].col(n);
            for (int i = 1; i < this->m_order - 1; i++) {
                VEC oldabvec = arma::vectorise(ab);
                VEC currentvec = ncp_factors[matorder[i]].col(n);
                ab.clear();
                ab = currentvec * oldabvec.t();
            }
            (*o_krp).col(n) = arma::vectorise(ab);
        }
    }
// caller must free
    Tensor rankk_tensor() {
        UWORD krpsize = arma::prod(this->m_dimensions);
        krpsize /= this->m_dimensions[0];
        MAT krpleavingzero = arma::zeros<MAT>(krpsize, this->m_k);
        krp_leave_out_one(0, &krpleavingzero);
        MAT lowranktensor(this->m_dimensions[0], krpsize);
        lowranktensor = this->ncp_factors[0] * krpleavingzero.t();
        Tensor rc(this->m_dimensions, lowranktensor.memptr());
        return rc;
    }

    void print() {
        std::cout << "order::" << this->m_order << "::k::" << this->m_k << std::endl;
        std::cout << "lambda::" << arma::prod(this->lambda) << std::endl;
        std::cout << "::dims::"  << std::endl << this->m_dimensions << std::endl;
        for (int i = 0; i < this->m_order; i++) {
            std::cout << i << "th factor" << std::endl << "=============" << std::endl;
            std::cout << this->ncp_factors[i];
        }
    }
    void print(const int i_n) {
        std::cout << i_n << "th factor" << std::endl << "=============" << std::endl;
        std::cout << this->ncp_factors[i_n];
    }
    void trans(NCPFactors &factor_t) {
        for (int i = 0; i < this->m_order; i++) {
            factor_t.set(i, this->ncp_factors[i].t());
        }
    }
    void normalize() {
        for (int i = 0; i < this->m_order; i++) {
            normalize(i);
        }
    }
}; // NCPFactors
}  // PLANC

#endif  //  NTF_CPFACTORS_HPP_