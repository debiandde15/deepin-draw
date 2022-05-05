#include "selectionitem.h"
#include "pageitem_p.h"
#include "itemgroup.h"
#include "cgraphicsitemevent.h"
#include "pagescene.h"
#include "pageview.h"
#include "global.h"


#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QGraphicsProxyWidget>

class SelectionItem::SelectionItem_private
{
public:
    explicit SelectionItem_private(SelectionItem *qq): q(qq)
    {}

    void initNodes()
    {
        for (int i = SelectionItemNode::Resize_LT; i <= SelectionItemNode::Rotation; ++i) {
            auto nd = new SelectionItemNode(i, q);
            q->addHandleNode(nd);
            nd->setVisible(false);
        }
    }

//    void setNodesVisible(bool b)
//    {
//        auto nodes = q->handleNodes();
//        foreach (auto node, nodes) {
//            node->setIconVisible(b);
//        }
//    }
    SelectionItem *q;

    QList<PageItem *> selectedItems;

    bool blockUpdateRect = false;

    bool showNodes = true;
};

SelectionItem::SelectionItem(PageScene *scene): QObject(scene), SelectionItem_d(new SelectionItem_private(this))
{
    d_SelectionItem()->initNodes();
    scene->addItem(this);
    this->setPageZValue(INT_MAX);

    connect(this, &SelectionItem::selectionChanged, this, [ = ](const QList<PageItem * > &selectedItems) {
        if (this->pageScene() == nullptr)
            return;

        if (this->page() == nullptr)
            return;

        emit this->pageScene()->selectionChanged(selectedItems);
        emit this->pageView()->selectionChanged(selectedItems);
    });
    d_SelectionItem()->initNodes();
}

SelectionItem::~SelectionItem()
{
//    delete SelectionItem_d;
//    SelectionItem_d = nullptr;
}

int SelectionItem::type() const
{
    return SelectionItemType;
}

SAttrisList SelectionItem::attributions()
{
    if (d_SelectionItem()->selectedItems.isEmpty())
        return SAttrisList();

    SAttrisList result = d_SelectionItem()->selectedItems.first()->attributions();
    foreach (auto p, d_SelectionItem()->selectedItems) {
        if (p == d_SelectionItem()->selectedItems.first()) {
            continue;
        }
        result = result.insected(p->attributions());
    }
    return result;
}

void SelectionItem::setAttributionVar(int attri, const QVariant &var, int phase)
{
    auto items = d_SelectionItem()->selectedItems;
    foreach (auto i, items) {
        if (i->attributions().haveAttribution(attri))
            i->setAttributionVar(attri, var, phase);
    }
}

void SelectionItem::select(PageItem *p)
{
    if (p == this)
        return;

    if (p->isItemSelected())
        return;

    if (GroupItem *group = p->pageGroup()) {
        select(group);
        return;
    }

    if (!d_SelectionItem()->selectedItems.contains(p)) {
        d_SelectionItem()->selectedItems.append(p);

        p->d_PageItem()->isSelected = true;
        p->setSelected(true);
        p->installChangeFilter(this);
        p->d_PageItem()->notifyItemChange(PageItemSelectionChanged, true);

        int count = d_SelectionItem()->selectedItems.count();
        if (count == 2) {
            foreach (auto p1, d_SelectionItem()->selectedItems) {
                p1->d_PageItem()->isMulSelected = true;
                p1->d_PageItem()->notifyItemChange(PageItemMulSelectionChanged, true);
                p1->update();
            }
        } else if (count > 2) {
            p->d_PageItem()->isMulSelected = true;
            p->d_PageItem()->notifyItemChange(PageItemMulSelectionChanged, true);
        }
        emit selectionChanged(d_SelectionItem()->selectedItems);
        p->update();

        updateRect();
    }
}

void SelectionItem::select(const QList<PageItem *> &items)
{
    {
        d_SelectionItem()->blockUpdateRect = true;
        QSignalBlocker blocker(this);
        foreach (auto p, items) {
            select(p);
        }
        d_SelectionItem()->blockUpdateRect = false;
    }

    updateRect();
    emit selectionChanged(d_SelectionItem()->selectedItems);
}
extern PageItem *_pageSceneSelectItem(PageScene *scene);
void SelectionItem_NotSelectHelper(PageItem *p)
{
    if (p->scene() != nullptr) {
        auto selectionItem = static_cast<SelectionItem *>(_pageSceneSelectItem(p->pageScene())) ;
        selectionItem->d_SelectionItem()->selectedItems.removeOne(p);
    }
    p->d_PageItem()->isSelected = false;
    p->d_PageItem()->isMulSelected = false;
}
void SelectionItem::notSelect(PageItem *p)
{
    if (p == this)
        return;

    if (!p->isItemSelected())
        return;

    if (GroupItem *group = p->pageGroup()) {
        notSelect(group);
        return;
    }

    if (d_SelectionItem()->selectedItems.contains(p)) {
        d_SelectionItem()->selectedItems.removeOne(p);
        p->d_PageItem()->isSelected = false;
        p->d_PageItem()->isMulSelected = false;
        p->setSelected(false);
        p->d_PageItem()->notifyItemChange(PageItemSelectionChanged, false);
        p->d_PageItem()->notifyItemChange(PageItemMulSelectionChanged, false);
        p->removeChangeFilter(this);
        int count = d_SelectionItem()->selectedItems.count();
        if (count == 1) {
            foreach (auto p1, d_SelectionItem()->selectedItems) {
                p1->d_PageItem()->isMulSelected = false;
                p1->d_PageItem()->notifyItemChange(PageItemMulSelectionChanged, false);
                p1->update();
            }
        }
        emit selectionChanged(d_SelectionItem()->selectedItems);
        p->update();

        updateRect();
    }
}

void SelectionItem::notSelect(const QList<PageItem *> &items)
{
    auto oldSelectedItems = d_SelectionItem()->selectedItems;

    {
        d_SelectionItem()->blockUpdateRect = true;
        QSignalBlocker blocker(this);
        foreach (auto p, items) {
            notSelect(p);
        }
        d_SelectionItem()->blockUpdateRect = false;
    }

    updateRect();

    if (oldSelectedItems != d_SelectionItem()->selectedItems) {
        emit selectionChanged(d_SelectionItem()->selectedItems);
    }
}

void SelectionItem::clear()
{
    auto items = d_SelectionItem()->selectedItems;
    notSelect(items);
}

QList<PageItem *> SelectionItem::selectedItems() const
{
    return d_SelectionItem()->selectedItems;
}

int SelectionItem::selectedCount() const
{
    return d_SelectionItem()->selectedItems.count();
}

PageItem *SelectionItem::singleSelectedItem() const
{
    if (d_SelectionItem()->selectedItems.count() == 1) {
        return d_SelectionItem()->selectedItems.first();
    }
    return nullptr;
}

void SelectionItem::setBlockUpdateRect(bool bBlock)
{
    d_SelectionItem()->blockUpdateRect = bBlock;
}

bool SelectionItem::contains(const QPointF &point) const
{
    return false;
}

void SelectionItem::loadUnit(const Unit &ut)
{
    if (ut.usage & UnitUsage_All_DontNewChild) {
        if (ut.chidren.count() == d_SelectionItem()->selectedItems.count()) {
            for (int i = 0; i < d_SelectionItem()->selectedItems.count(); ++i) {
                auto child = d_SelectionItem()->selectedItems.at(i);
                child->loadItemUnit(ut.chidren.at(i));
            }
        } else {
            qWarning() << "load Unit failed because ut.children count not same with item children when unit usage=UnitUsage_All_DontNewChild";
            qWarning() << "ut.children count = " << ut.chidren.count() << "items children count = " << childPageItems().count();
        }
    }
    RectBaseItem::loadUnit(ut);
}

Unit SelectionItem::getUnit(int use) const
{
    Unit ut = RectBaseItem::getUnit(use);
    ut.usage = use;
    if ((ut.usage & UnitUsage_All_NewChild) || (ut.usage & UnitUsage_All_DontNewChild)) {
        for (int i = 0; i < d_SelectionItem()->selectedItems.count(); ++i) {
            auto child = d_SelectionItem()->selectedItems.at(i);
            ut.chidren.append(child->getItemUnit(use));
        }
    }
    return ut;
}

void SelectionItem::updateRect()
{
    if (operatingType() == 3) {
        return;
    }

    if (d_SelectionItem()->blockUpdateRect) {
        return;
    }

    preparePageItemGeometryChange();
    QRectF rect(0, 0, 0, 0);

    auto count = d_SelectionItem()->selectedItems.size();
    if (count > 1) {
        setFlag(ItemHasNoContents, false);
        d_SelectionItem()->showNodes = true;
        foreach (PageItem *pItem, d_SelectionItem()->selectedItems) {
            if (pItem != nullptr) {
                rect = rect.united(pItem->mapRectToScene(pItem->selectedRectangle()));
            }
        }
        this->setTransformOriginPoint(rect.center());
        this->setRotation(0);

        resetTransform();

        d_PageItem()->_roteAgnel = 0;

        rect = mapFromScene(rect).boundingRect();
    } else if (count == 1) {

        PageItem *pItem = d_SelectionItem()->selectedItems.first();

        rect = pItem->selectedRectangle();

        setFlag(ItemHasNoContents, !(pItem->isSingleSelectionBorderVisible()));
        d_SelectionItem()->showNodes = (pItem->nodeCount() == 0);

        this->setPos(0, 0);
        this->setScale(1.0);
        this->setRotation(0);
        this->resetTransform();
        this->setTransform(pItem->sceneTransform());

        d_PageItem()->_roteAgnel = pItem->drawRotation();
    }
    setRect(rect);
}

void SelectionItem::paintSelf(QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    Q_UNUSED(option)

    painter->setClipping(false);
    QPen pen;
    pen.setWidthF(1 / option->levelOfDetailFromTransform(painter->worldTransform()));
    pen.setColor(QColor(187, 187, 187));

    painter->setPen(pen);
    painter->setBrush(QBrush(Qt::NoBrush)/*QBrush(QColor(255, 0, 0, 100))*/);
    painter->drawRect(this->orgRect());
    painter->setClipping(true);
}

void SelectionItem::operatingBegin(PageItemEvent *event)
{
    d_SelectionItem()->blockUpdateRect =  true;
    foreach (PageItem *pItem, d_SelectionItem()->selectedItems) {
        QTransform thisToItem = this->itemTransform(pItem);
        PageItemEvent *childEvent = event->creatTransDuplicate(thisToItem, pItem->orgRect().size());
        childEvent->setItem(pItem);
        pItem->operatingBegin(childEvent);
        delete childEvent;
    }
    d_PageItem()->operatingBegin_helper(event);
}

void SelectionItem::operating(PageItemEvent *event)
{
    bool accept = testOpetating(event);
    if (accept) {
        foreach (PageItem *pItem, d_SelectionItem()->selectedItems) {
            //得到将自身坐标系映射到其他图元pItem坐标系的矩阵
            QTransform thisToItem = this->itemTransform(pItem);
            PageItemEvent *childEvent = event->creatTransDuplicate(thisToItem, pItem->orgRect().size());
            childEvent->setItem(pItem);
            //将自身要做的转换矩阵映射到pItem上
            QTransform childDoTrans = thisToItem.inverted() * event->trans() * thisToItem;
            childEvent->setTrans(childDoTrans);
            pItem->operating(childEvent);
            delete childEvent;
        }
        d_PageItem()->operating_helper(event);
    }
}

bool SelectionItem::testOpetating(PageItemEvent *event)
{
    //1.先判断自身是否能接受这个操作事件
    bool accept = d_PageItem()->testOpetating_helper(event);
    if (accept) {
        //2.再判断孩子们是否能接受这个操作事件
        if (event->type() == PageItemEvent::EScal) {
            //auto doItems = items(true);
            foreach (PageItem *pItem, d_SelectionItem()->selectedItems) {
                QTransform thisToItem = this->itemTransform(pItem);
                PageItemEvent *childEvent = event->creatTransDuplicate(thisToItem, pItem->orgRect().size());
                childEvent->setItem(pItem);
                childEvent->setDriverEvent(event);
                QTransform childDoTrans = thisToItem.inverted() * event->trans() * thisToItem;
                childEvent->setTrans(childDoTrans);
                bool isItemAccept = pItem->/*d_PageItem()->testOpetating_helper*/testOpetating(childEvent);
                if (!isItemAccept) {
                    accept = false;
                    delete childEvent;
                    break;
                }
                delete childEvent;
            }
        }
    }
    return accept;
}

void SelectionItem::operatingEnd(PageItemEvent *event)
{
    foreach (PageItem *pItem, d_SelectionItem()->selectedItems) {
        QTransform thisToItem = this->itemTransform(pItem);
        PageItemEvent *childEvent = event->creatTransDuplicate(thisToItem, pItem->orgRect().size());
        childEvent->setItem(pItem);
        childEvent->setTrans(thisToItem.inverted() * event->trans() * thisToItem);
        pItem->operatingEnd(childEvent);
        delete childEvent;
    }
    d_PageItem()->operatingEnd_helper(event);
    d_SelectionItem()->blockUpdateRect = false;
}

//void SelectionItem::loadUnit(const Unit &ut)
//{
//    switch (ut.usage) {
//    case UnitUsage_OnlySelf: {
//        break;
//    }
//    case UnitUsage_All_NewChild: {
//        for (int i = 0; i < ut.chidren.count(); ++i) {
//            auto childUt = ut.chidren.at(i);
//            this->addChild(creatItemInstance(childUt.head.dataType, childUt));
//        }
//        break;
//    }
//    case UnitUsage_All_DontNewChild: {
//        if (ut.chidren.count() == d_SelectionItem()->selectedItems.count()) {
//            for (int i = 0; i < d_SelectionItem()->selectedItems.count(); ++i) {
//                auto child = d_SelectionItem()->selectedItems.at(i);
//                child->loadItemUnit(ut.chidren.at(i));
//            }
//        } else {
//            qWarning() << "load Unit failed because ut.children count not same with selectedItems when unit usage=UnitUsage_All_DontNewChild";
//            qWarning() << "ut.children count = " << ut.chidren.count() << "selectedItems count = " << d_SelectionItem()->selectedItems.count();
//        }
//        break;
//    }
//    default:
//        break;
//    }
//    return RectBaseItem::loadUnit(ut);
//}

//Unit SelectionItem::getUnit(int use) const
//{
//    Unit ut = RectBaseItem::getUnit(use);
//    ut.usage = use;
//    switch (ut.usage) {
//    case UnitUsage_OnlySelf: {
//        break;
//    }
//    case UnitUsage_All_NewChild:
//    case UnitUsage_All_DontNewChild: {
//        for (int i = 0; i < d_SelectionItem()->selectedItems.count(); ++i) {
//            auto child = d_SelectionItem()->selectedItems.at(i);
//            ut.chidren.append(child->getItemUnit(use));
//        }
//        break;
//    }
//    }
//    return ut;
//}

bool SelectionItem::pageItemChangeFilter(PageItem *item, int changeType, QVariant &value)
{
    if (changeType == PageItemSelectionChanged || changeType == ItemPositionHasChanged || changeType == ItemTransformHasChanged
            || changeType == PageItemRectChanged) {
        //qWarning() << "changeType ========== " << changeType;
        updateRect();
        QMetaObject::invokeMethod(this, [ = ]() {
            foreach (auto handle, handleNodes()) {
                handle->update();
            }
        }, Qt::DirectConnection);
    }
    return RectBaseItem::pageItemChangeFilter(item, changeType, value);
}

QPainterPath SelectionItem::calOrgShapeBaseRect(const QRectF &rect) const
{
    QPainterPath path;
    path.addRect(rect);
    return path;
}

void SelectionItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (pageView() == nullptr) {
        return;
    }
    if (pageView()->activeProxItem() != nullptr) {
        QGraphicsItem *item = pageView()->activeProxItem();
        event->setPos(item->mapFromScene(event->scenePos()));
        scene()->sendEvent(item, event);
        return;
    }
    showWidgetInScreen(pageView()->menu(), QCursor::pos());
}

//void SelectionItemNode::parentItemChanged(int doChange, const QVariant &value)
//{
//    //int tp;
//    if (doChange == PageItem::PageItemRectChanged) {
//        bool show = false;
//        auto gp = static_cast<SelectionItem *>(parentPageItem());
//        if (gp->selectedCount() == 1) {
//            auto selectedItems = gp->selectedItems();
//            auto item = selectedItems.first();
//            show = item->nodeCount() == 0;
//        } else {
//            show = gp->selectedCount() > 0;
//        }
//        if (show) {
//            setNodePos();
//        }
//        //qWarning() << "setvisble ======== " << show;
//        setVisible(show);
//    }
//}

bool SelectionItemNode::isVisbleCondition() const
{
    //auto gp = static_cast<SelectionItem *>(parentPageItem());
    //return !(gp->flags()&ItemHasNoContents);

    auto gp = static_cast<SelectionItem *>(parentPageItem());
    return gp->d_SelectionItem()->showNodes;
}
