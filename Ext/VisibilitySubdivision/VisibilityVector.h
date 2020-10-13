/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION

#ifndef MINSG_VISIBILITYSUBDIVISION_VISIBILITYVECTOR_H_
#define MINSG_VISIBILITYSUBDIVISION_VISIBILITYVECTOR_H_

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

namespace Util {
template<typename Type> class WrapperAttribute;
}
namespace MinSG {
class GeometryNode;
namespace SceneManagement {
class SceneManager;
}

namespace VisibilitySubdivision {
/**
 * Storage class for the mapping of nodes to their benefits. The
 * benefits are a natural number and can represent the number of visible
 * pixels for example.
 *
 * @author Benjamin Eikel
 * @date 2009-02-18, benefits and typedefs added 2009-08-18, costs
 * removed from map 2010-01-15
 */
class VisibilityVector {
	public:
		typedef GeometryNode * node_ptr;
		typedef uint32_t costs_t;
		typedef uint32_t benefits_t;
		typedef std::pair<node_ptr, benefits_t> node_benefits_pair_t;

		//! Equality comparison
		MINSGAPI bool operator==(const VisibilityVector & other) const;

		/**
		 * Set the benefits of one node. If the node was
		 * already added before, the data will be updated.
		 *
		 * @param node Pointer to the node.
		 * @param nodeBenefits Benefits of the node.
		 */
		MINSGAPI void setNode(node_ptr node, benefits_t nodeBenefits);

		/**
		 * Remove a node from the vector
		 *
		 * @param node Pointer to the node.
		 */
		MINSGAPI void removeNode(node_ptr node);

		/**
		 * Get the costs of one node.
		 *
		 * @param node Pointer to the node.
		 * @return Costs of the node, or 0 if the node is not stored here.
		 */
		MINSGAPI costs_t getCosts(node_ptr node) const;

		/**
		 * Get the benefits of one node.
		 *
		 * @param node Pointer to the node.
		 * @return Benefits of the node, or 0 if the node is not stored here.
		 */
		MINSGAPI benefits_t getBenefits(node_ptr node) const;

		/**
		 * Increase the benefits of a node. If the node has not been stored here
		 * before, add the node with the given benefits.
		 * 
		 * The result is equivalent to the following:
		 * @code
		 * benefits_t oldBenefits = vv.getBenefits(node);
		 * vv.setNode(node, oldBenefits + benefitsIncr);
		 * @endcode
		 * 
		 * @param node Pointer to the node
		 * @param benefitsIncr Amount that the benefits of the node is increased
		 * @return The benefits of the node before the increase, or zero if the
		 * node has not been stored here before.
		 */
		MINSGAPI benefits_t increaseBenefits(node_ptr node, benefits_t benefitsIncr);

		/**
		 * Return the number of indices for the data stored here.
		 * This value is useful when iteratively accessing the data stored here.
		 *
		 * @return Number of indices
		 * @note This is number is one larger than what is allowed as parameter
		 * in @a getNode(uint32_t), @a getCosts(uint32_t), or @a getBenefits(uint32_t).
		 */
		MINSGAPI uint32_t getIndexCount() const;

		/**
		 * Get a node.
		 *
		 * @param index Index of the node from [0, @a getIndexCount)
		 * @return Pointer to the node, or @c nullptr if the index is invalid.
		 */
		MINSGAPI node_ptr getNode(uint32_t index) const;

		/**
		 * Get the costs of one node.
		 *
		 * @param index Index of the node from [0, @a getIndexCount)
		 * @return Costs of the node, or 0 if the index is invalid.
		 */
		MINSGAPI costs_t getCosts(uint32_t index) const;

		/**
		 * Get the benefits of one node.
		 *
		 * @param index Index of the node from [0, @a getIndexCount)
		 * @return Benefits of the node, or 0 if the index is invalid.
		 */
		MINSGAPI benefits_t getBenefits(uint32_t index) const;

		/**
		 * Get the sum of all costs stored in this vector.
		 *
		 * @return Overall costs.
		 */
		MINSGAPI costs_t getTotalCosts() const;

		/**
		 * Get the sum of all costs stored in this vector.
		 *
		 * @return Overall costs.
		 */
		MINSGAPI benefits_t getTotalBenefits() const;

		/**
		 * Return the number of nodes stored in this vector.
		 *
		 * @return Number of nodes
		 */
		MINSGAPI std::size_t getVisibleNodeCount() const;

		/**
		 * Return a minimal visibility vector calculated from the values of two visibility vectors.
		 * Only if an object exists in both of them, the minimal costs and benefits of this object from both vectors are returned.
		 *
		 * @param vv1 First visibility vector.
		 * @param vv2 Second visibility vector.
		 * @return Visibility vector containing the minimal vector.
		 */
		MINSGAPI static VisibilityVector makeMin(const VisibilityVector & vv1, const VisibilityVector & vv2);

		/**
		 * Return a maximal visibility vector calculated from the values of two visibility vectors.
		 * If an object exists in one of them, the maximal costs and benefits of this object from both vectors are returned.
		 *
		 * @param vv1 First visibility vector.
		 * @param vv2 Second visibility vector.
		 * @return Visibility vector containing the maximal vector.
		 */
		MINSGAPI static VisibilityVector makeMax(const VisibilityVector & vv1, const VisibilityVector & vv2);

		/**
		 * Return a difference visibility vector calculated from the values of two visibility vectors.
		 * The difference contains the values that are in the first visibility vector but not in the second.
		 *
		 * @param vv1 First visibility vector.
		 * @param vv2 Second visibility vector.
		 * @return Visibility vector containing the difference vector.
		 */
		MINSGAPI static VisibilityVector makeDifference(const VisibilityVector & vv1, const VisibilityVector & vv2);

		/**
		 * Return a symmetric difference visibility vector calculated from the values of two visibility vectors.
		 * The symmetric difference contains the values that are in either of the visibility vectors and not in both.
		 *
		 * @param vv1 First visibility vector.
		 * @param vv2 Second visibility vector.
		 * @return Visibility vector containing the symmetric difference vector.
		 */
		MINSGAPI static VisibilityVector makeSymmetricDifference(const VisibilityVector & vv1, const VisibilityVector & vv2);

		/**
		 * Return a visibility vector calculated from the values of three visibility vectors.
		 * If an entry is contained in a visibility vector, it is weighted with the given weight for that visibility vector.
		 * If an entry is not contained in a visibility vector, it is weighted with zero for that visibility vector.
		 * The resulting benefits for that entry will be the weighted sum of the three benefits.
		 *
		 * @param w1 Weight for the objects of the first visibility vector.
		 * @param vv1 First visibility vector.
		 * @param w2 Weight for the objects of the second visibility vector.
		 * @param vv2 Second visibility vector.
		 * @param w3 Weight for the objects of the third visibility vector.
		 * @param vv3 Third visibility vector.
		 * @param[out] result Visibility vector containing the weighted visibility vector.
		 */
		MINSGAPI static VisibilityVector makeWeightedThree(float w1, const VisibilityVector & vv1,
												  float w2, const VisibilityVector & vv2,
												  float w3, const VisibilityVector & vv3);

		/**
		 * Return the difference in costs and benefits between two visibility vectors.
		 *
		 * @param[in] vv1 First visibility vector.
		 * @param[in] vv2 Second visibility vector.
		 * @param[out] costsDiff Difference in costs.
		 * @param[out] benefitsDiff Difference in benefits.
		 * @param[out] sameCount Number of entries pointing to the same object.
		 */
		MINSGAPI static void diff(const VisibilityVector & vv1, const VisibilityVector & vv2,
						 costs_t & costsDiff, benefits_t & benefitsDiff, std::size_t & sameCount);

		/**
		 * Return a human-readable representation.
		 *
		 * @return Textual representation of this object.
		 */
		MINSGAPI std::string toString() const;

		/**
		 * Calculate the amount of memory that is required to store the
		 * visibility vector.
		 * 
		 * @return Overall amount of memory in bytes
		 */
		MINSGAPI size_t getMemoryUsage() const;

		//! @name Serialization
		//@{

		/**
		 * Write a visibility vector to a stream.
		 * 
		 * @param out Output stream
		 * @param sceneManager Reference to the scene manager that is needed to
		 * look up registered nodes
		 */
		MINSGAPI void serialize(std::ostream & out,
					   const SceneManagement::SceneManager & sceneManager) const;

		/**
		 * Read a visibility vector from a stream.
		 * 
		 * @param in Input stream
		 * @param sceneManager Reference to the scene manager that is needed to
		 * look up registered nodes
		 * @return New visibility vector
		 */
		MINSGAPI static VisibilityVector unserialize(std::istream & in,
											const SceneManagement::SceneManager & sceneManager);
		//@}

	private:
		//! Storage of visibility data consisting of pairs of nodes and their benefits.
		std::vector<node_benefits_pair_t> visibility;
};

// Class to store a VisibilityVector as GenericAttribute
typedef Util::WrapperAttribute<VisibilityVector> VisibilityVectorAttribute;

}
}

#endif /* MINSG_VISIBILITYSUBDIVISION_VISIBILITYVECTOR_H_ */
#endif /* MINSG_EXT_VISIBILITY_SUBDIVISION */
