#include "SubsurfaceTree.hpp"
#include "../events/Events.hpp"

void addSurfaceGlobalOffset(SSurfaceTreeNode* node, int* lx, int* ly) {
    *lx += node->pSurface->sx;
    *ly += node->pSurface->sy;

    if (node->offsetfn) {
        // This is the root node
        RASSERT(!node->pSubsurface, "Node had no subsurface!");
        node->offsetfn(node->globalOffsetData, lx, ly);
    } else {
        RASSERT(node->pSubsurface, "Node had no subsurface!");

        *lx += node->pSubsurface->pSubsurface->current.x;
        *ly += node->pSubsurface->pSubsurface->current.y;

        addSurfaceGlobalOffset(node->pParent, lx, ly);
    }
}

SSurfaceTreeNode* createTree(wlr_surface* pSurface) {
    SubsurfaceTree::surfaceTreeNodes.push_back(SSurfaceTreeNode());

    const auto PNODE = &SubsurfaceTree::surfaceTreeNodes.back();

    PNODE->pSurface = pSurface;

    PNODE->hyprListener_newSubsurface.initCallback(&pSurface->events.new_subsurface, &Events::listener_newSubsurfaceNode, PNODE, "SurfaceTreeNode");
    PNODE->hyprListener_commit.initCallback(&pSurface->events.commit, &Events::listener_commitSubsurface, PNODE, "SurfaceTreeNode");
    PNODE->hyprListener_destroy.initCallback(&pSurface->events.destroy, &Events::listener_destroySubsurfaceNode, PNODE, "SurfaceTreeNode");

    wlr_subsurface* wlrSubsurface;
    wl_list_for_each(wlrSubsurface, &pSurface->current.subsurfaces_below, current.link) {
        Events::listener_newSubsurfaceNode(PNODE, wlrSubsurface);
    }
    wl_list_for_each(wlrSubsurface, &pSurface->current.subsurfaces_above, current.link) {
        Events::listener_newSubsurfaceNode(PNODE, wlrSubsurface);
    }

    return PNODE;
}

SSurfaceTreeNode* createSubsurfaceNode(SSurfaceTreeNode* pParent, SSubsurface* pSubsurface, wlr_surface* surface) {
    const auto PNODE = createTree(surface);
    PNODE->pParent = pParent;
    PNODE->pSubsurface = pSubsurface;

    return PNODE;
}

SSurfaceTreeNode* SubsurfaceTree::createTreeRoot(wlr_surface* pSurface, applyGlobalOffsetFn fn, void* data) {
    const auto PNODE = createTree(pSurface);
    PNODE->offsetfn = fn;
    PNODE->globalOffsetData = data;

    return PNODE;
}

void destroySubsurface(SSubsurface* pSubsurface);

void SubsurfaceTree::destroySurfaceTree(SSurfaceTreeNode* pNode) {
    bool exists = false;
    for (auto& n : surfaceTreeNodes) {
        if (&n == pNode) {
            exists = true;
            break;
        }
    }

    if (!exists) {
	    Debug::log(ERR, "Tried to remove a SurfaceTreeNode that doesn't exist?? (Node %x)", pNode);
	    return;
    }

    for (auto& c : pNode->childSubsurfaces)
        destroySubsurface(&c);

    pNode->childSubsurfaces.clear();

    pNode->hyprListener_commit.removeCallback();
    pNode->hyprListener_destroy.removeCallback();
    pNode->hyprListener_newSubsurface.removeCallback();

    surfaceTreeNodes.remove(*pNode);

    Debug::log(LOG, "SurfaceTree Node removed");
}

void destroySubsurface(SSubsurface* pSubsurface) {
    if (pSubsurface->pChild) {
        SubsurfaceTree::destroySurfaceTree(pSubsurface->pChild);
        pSubsurface->pChild = nullptr;
    }

    pSubsurface->hyprListener_destroy.removeCallback();
    pSubsurface->hyprListener_map.removeCallback();
    pSubsurface->hyprListener_unmap.removeCallback();
}

//
// Subsurface listeners
//

void Events::listener_newSubsurfaceNode(void* owner, void* data) {
    SSurfaceTreeNode* pNode = (SSurfaceTreeNode*)owner;

    const auto PSUBSURFACE = (wlr_subsurface*)data;

    pNode->childSubsurfaces.push_back(SSubsurface());
    const auto PNEWSUBSURFACE = &pNode->childSubsurfaces.back();

    Debug::log(LOG, "Added a new subsurface %x", PSUBSURFACE);

    PNEWSUBSURFACE->pSubsurface = PSUBSURFACE;
    PNEWSUBSURFACE->pParent = pNode;

    PNEWSUBSURFACE->hyprListener_map.initCallback(&PSUBSURFACE->events.map, &Events::listener_mapSubsurface, PNEWSUBSURFACE, "Subsurface");
    PNEWSUBSURFACE->hyprListener_unmap.initCallback(&PSUBSURFACE->events.unmap, &Events::listener_unmapSubsurface, PNEWSUBSURFACE, "Subsurface");
    PNEWSUBSURFACE->hyprListener_destroy.initCallback(&PSUBSURFACE->events.destroy, &Events::listener_destroySubsurface, PNEWSUBSURFACE, "Subsurface");

    wlr_subsurface* existingWlrSubsurface;
    wl_list_for_each(existingWlrSubsurface, &PSUBSURFACE->surface->current.subsurfaces_below, current.link) {
        listener_newSubsurfaceNode(pNode, existingWlrSubsurface);
    }
    wl_list_for_each(existingWlrSubsurface, &PSUBSURFACE->surface->current.subsurfaces_above, current.link) {
        listener_newSubsurfaceNode(pNode, existingWlrSubsurface);
    }
}

void Events::listener_mapSubsurface(void* owner, void* data) {
    SSubsurface* subsurface = (SSubsurface*)owner;

    Debug::log(LOG, "Subsurface %x mapped", subsurface->pSubsurface);

    subsurface->pChild = createSubsurfaceNode(subsurface->pParent, subsurface, subsurface->pSubsurface->surface);
}

void Events::listener_unmapSubsurface(void* owner, void* data) {
    SSubsurface* subsurface = (SSubsurface*)owner;

    Debug::log(LOG, "Subsurface %x unmapped", subsurface);

    if (subsurface->pChild) {
        const auto PNODE = subsurface->pChild;

        int lx = 0, ly = 0;
        addSurfaceGlobalOffset(PNODE, &lx, &ly);

        wlr_box extents = {0};
        wlr_surface_get_extends(PNODE->pSurface, &extents);

        extents.x += lx;
        extents.y += ly;

        SubsurfaceTree::destroySurfaceTree(subsurface->pChild);
        subsurface->pChild = nullptr;
    }
}

void Events::listener_commitSubsurface(void* owner, void* data) {
    SSurfaceTreeNode* pNode = (SSurfaceTreeNode*)owner;

    int lx = 0, ly = 0;

    addSurfaceGlobalOffset(pNode, &lx, &ly);
}

void Events::listener_destroySubsurface(void* owner, void* data) {
    SSubsurface* subsurface = (SSubsurface*)owner;

    if (subsurface->pChild)
        listener_destroySubsurfaceNode(subsurface->pChild, nullptr);

    Debug::log(LOG, "Subsurface %x destroyed", subsurface);

    subsurface->hyprListener_destroy.removeCallback();
    subsurface->hyprListener_map.removeCallback();
    subsurface->hyprListener_unmap.removeCallback();

    subsurface->pParent->childSubsurfaces.remove(*subsurface);
}

void Events::listener_destroySubsurfaceNode(void* owner, void* data) {
    SSurfaceTreeNode* pNode = (SSurfaceTreeNode*)owner;

    Debug::log(LOG, "Subsurface Node %x destroyed", pNode);

    for (auto& c : pNode->childSubsurfaces)
        destroySubsurface(&c);

    pNode->hyprListener_commit.removeCallback();
    pNode->hyprListener_newSubsurface.removeCallback();
    pNode->hyprListener_destroy.removeCallback();

    SubsurfaceTree::surfaceTreeNodes.remove(*pNode);
}
