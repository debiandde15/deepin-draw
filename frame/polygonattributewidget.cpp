/*
 * Copyright (C) 2019 ~ %YEAR% Deepin Technology Co., Ltd.
 *
 * Author:     Renran
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "polygonattributewidget.h"

#include <DLabel>
#include <DFontSizeManager>



#include <QHBoxLayout>
#include <QButtonGroup>

#include "widgets/csidewidthwidget.h"
#include "widgets/toolbutton.h"
#include "widgets/bigcolorbutton.h"
#include "widgets/bordercolorbutton.h"
#include "widgets/seperatorline.h"
#include "utils/cvalidator.h"
#include "drawshape/cdrawparamsigleton.h"


const int BTN_SPACING = 6;
const int SEPARATE_SPACING = 5;
const int TEXT_SIZE = 12;
//DWIDGET_USE_NAMESPACE

PolygonAttributeWidget::PolygonAttributeWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    initConnection();
}



PolygonAttributeWidget::~PolygonAttributeWidget()
{

}

void PolygonAttributeWidget::changeButtonTheme()
{
    m_sideWidthWidget->changeButtonTheme();
    m_sepLine->updateTheme();
}

void PolygonAttributeWidget::initUI()
{
    //DFontSizeManager::instance()->bind(this, DFontSizeManager::T1);

    m_fillBtn = new BigColorButton(this);
    DLabel *fillLabel = new DLabel(this);
    fillLabel->setText(tr("填充"));
    QFont ft;
    ft.setPixelSize(TEXT_SIZE);
    fillLabel->setFont(ft);

    m_strokeBtn = new BorderColorButton(this);

    DLabel *strokeLabel = new DLabel(this);
    strokeLabel->setText(tr("描边"));
    strokeLabel->setFont(ft);

    m_sepLine = new SeperatorLine(this);
    DLabel *lwLabel = new DLabel(this);
    lwLabel->setText(tr("描边粗细"));
    lwLabel->setFont(ft);

    m_sideWidthWidget = new CSideWidthWidget(this);


    DLabel *sideNumLabel = new DLabel(this);
    sideNumLabel->setText(tr("侧边数"));
    sideNumLabel->setFont(ft);

    m_sideNumSlider = new DSlider(Qt::Horizontal, this);
//    m_sideNumSlider->slider()->setSingleStep(1);
    m_sideNumSlider->setMinimum(4);
    m_sideNumSlider->setMaximum(200);
    m_sideNumSlider->setMinimumWidth(120);


    m_sideNumEdit = new DLineEdit(this);
//    m_sideNumEdit->lineEdit()->setValidator(new QRegExpValidator(QRegExp("^(()|([4-9]{1})|([1]{1}[0]{0,1}))$")));
//    m_sideNumEdit->setValidator(new CIntValidator(4, 10));
    m_sideNumEdit->setClearButtonEnabled(false);
    m_sideNumEdit->setFixedWidth(40);
    m_sideNumEdit->setText(QString::number(m_sideNumSlider->value()));
    m_sideNumEdit->setFont(ft);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(BTN_SPACING);
    layout->addStretch();
    layout->addWidget(m_fillBtn);
    layout->addWidget(fillLabel);
    layout->addWidget(m_strokeBtn);
    layout->addWidget(strokeLabel);
    layout->addSpacing(SEPARATE_SPACING);
    layout->addWidget(m_sepLine);
    layout->addSpacing(SEPARATE_SPACING);
    layout->addWidget(lwLabel);
    layout->addWidget(m_sideWidthWidget);
    layout->addSpacing(SEPARATE_SPACING);
    layout->addWidget(sideNumLabel);
    layout->addWidget(m_sideNumSlider);
    layout->addSpacing(SEPARATE_SPACING);
    layout->addWidget(m_sideNumEdit);

    layout->addStretch();
    setLayout(layout);
}

void PolygonAttributeWidget::initConnection()
{
    connect(m_fillBtn, &BigColorButton::btnCheckStateChanged, this, [ = ](bool show) {
        m_strokeBtn->resetChecked();
        emit showColorPanel(DrawStatus::Fill, getBtnPosition(m_fillBtn), show);

    });
    connect(m_strokeBtn, &BorderColorButton::btnCheckStateChanged, this, [ = ](bool show) {
        m_fillBtn->resetChecked();
        emit showColorPanel(DrawStatus::Stroke,  getBtnPosition(m_strokeBtn), show);
    });

    connect(this, &PolygonAttributeWidget::resetColorBtns, this, [ = ] {
        m_fillBtn->resetChecked();
        m_strokeBtn->resetChecked();
    });

    connect(m_sideNumSlider, &DSlider::valueChanged, this, [ = ](int value) {
        m_sideNumEdit->setText(QString::number(value));
    });

    ///线宽
    connect(m_sideWidthWidget, &CSideWidthWidget::signalSideWidthChange, this, [ = ] () {
        emit signalPolygonAttributeChanged();
    });

    ///多边形边数
    connect(m_sideNumSlider, &DSlider::valueChanged, this, [ = ](int value) {
        m_sideNumEdit->setText(QString::number(value));
        CDrawParamSigleton::GetInstance()->setSideNum(value);
        emit signalPolygonAttributeChanged();
    });

    connect(m_sideNumEdit, &DLineEdit::textEdited, this, [ = ](const QString & str) {
        if (str.isEmpty() || str == "") {
            return ;
        }
        int value = str.trimmed().toInt();
        if (value == 1) {
            return ;
        }
        m_sideNumSlider->setValue(value);
    });

    connect(m_sideNumEdit, &DLineEdit::editingFinished, this, [ = ]() {
        QString str = m_sideNumEdit->text().trimmed();
        int minvalue = m_sideNumSlider->minimum();
        int value = 0;
        if (str.isEmpty() || str == "") {
            value = minvalue;
        } else {
            if ( str.toInt() == 1) {
                value = minvalue;
            }
        }

        if (value == minvalue ) {
            m_sideNumEdit->setText(QString::number(minvalue));
            m_sideNumSlider->setValue(minvalue);
        }
    });
}

void PolygonAttributeWidget::updatePolygonWidget()
{
    m_fillBtn->updateConfigColor();
    m_strokeBtn->updateConfigColor();
    m_sideWidthWidget->updateSideWidth();

    int sideNum = CDrawParamSigleton::GetInstance()->getSideNum();

    if (sideNum != m_sideNumSlider->value()) {
        m_sideNumSlider->setValue(sideNum);
        m_sideNumEdit->setText(QString("%1").arg(sideNum));
    }
}

QPoint PolygonAttributeWidget::getBtnPosition(const DPushButton *btn)
{
    QPoint btnPos = mapToGlobal(btn->pos());
    QPoint pos(btnPos.x() + btn->width() / 2,
               btnPos.y() + btn->height());

    return pos;
}
