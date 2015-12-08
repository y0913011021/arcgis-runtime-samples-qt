// Copyright 2015 Esri.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "FeatureLayerQuery.h"

#include "Map.h"
#include "MapQuickView.h"
#include "FeatureLayer.h"
#include "Basemap.h"
#include "SpatialReference.h"
#include "ServiceFeatureTable.h"
#include "Viewpoint.h"
#include "Point.h"
#include "SimpleLineSymbol.h"
#include "SimpleFillSymbol.h"
#include "SimpleRenderer.h"
#include "QueryParameters.h"
#include "FeatureQueryResult.h"
#include "Feature.h"
#include <QUrl>
#include <QColor>
#include <QList>

using namespace Esri::ArcGISRuntime;

FeatureLayerQuery::FeatureLayerQuery(QQuickItem* parent) :
    QQuickItem(parent),
    m_map(nullptr),
    m_mapView(nullptr),
    m_featureLayer(nullptr),
    m_featureTable(nullptr),
    m_initialized(false),
    m_queryResultsCount(0)
{
}

FeatureLayerQuery::~FeatureLayerQuery()
{
}

void FeatureLayerQuery::componentComplete()
{
    QQuickItem::componentComplete();

    // find QML MapView component
    m_mapView = findChild<MapQuickView*>("mapView");
    m_mapView->setWrapAroundMode(WrapAroundMode::Disabled);

    // Create a map using the topographic basemap
    m_map = new Map(Basemap::topographic(this), this);
    m_map->setInitialViewpoint(Viewpoint(Point(-11e6, 5e6, SpatialReference(102100)), 9e7));

    // Set map to map view
    m_mapView->setMap(m_map);

    // create the feature table
    m_featureTable = new ServiceFeatureTable(QUrl("https://sampleserver6.arcgisonline.com/arcgis/rest/services/USA/MapServer/2"), this);
    // create the feature layer using the feature table
    m_featureLayer = new FeatureLayer(m_featureTable, this);

    // line symbol for the outline
    SimpleLineSymbol* outline = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, QColor("black"), 2.0f, this);
    // fill symbol
    SimpleFillSymbol* sfs = new SimpleFillSymbol(SimpleFillSymbolStyle::Solid, QColor(255, 255, 0, 153), outline, this);
    // create the renderer using the symbology created above
    SimpleRenderer* renderer = new SimpleRenderer(sfs, this);
    // set the renderer for the feature layer
    m_featureLayer->setRenderer(renderer);

    // add the feature layer to the map
    m_map->operationalLayers()->append(m_featureLayer);

    // connect signals
    connectSignals();
}

void FeatureLayerQuery::connectSignals()
{
    // iterate over the query results once the query is done
    connect(m_featureTable, &ServiceFeatureTable::queryFeaturesCompleted, [this](QUuid, QSharedPointer<Esri::ArcGISRuntime::FeatureQueryResult> queryResult)
    {
        if (!queryResult->iterator().hasNext())
        {
            m_queryResultsCount = 0;
            emit queryResultsCountChanged();
            return;
        }

        // clear any existing selection
        m_featureLayer->clearSelection();
        QList<Feature*> features;

        // iterate over the result object
        while(queryResult->iterator().hasNext())
        {
            // add each feature to the list
            features.append(queryResult->iterator().next(this));
        }

        // select the feature
        m_featureLayer->selectFeatures(features);
        // zoom to the first feature
        m_mapView->setViewpointGeometry(features.at(0)->geometry(), 200);
        // set the count for QML property
        m_queryResultsCount = features.count();
        emit queryResultsCountChanged();
    });

    connect(m_featureTable, &ServiceFeatureTable::loadStatusChanged,[this](LoadStatus loadStatus)
    {
        loadStatus == LoadStatus::Loaded ? m_initialized = true : m_initialized = false;
        emit layerInitializedChanged();
    });
}

bool FeatureLayerQuery::layerInitialized() const
{
    return m_initialized;
}

void FeatureLayerQuery::runQuery(QString whereClause)
{
    // create a query parameter object and set the where clause
    QueryParameters queryParams;
    queryParams.setWhereClause(QString("STATE_NAME LIKE \'" + whereClause.toUpper() + "%\'"));
    m_featureTable->queryFeatures(queryParams);
}

int FeatureLayerQuery::queryResultsCount() const
{
    return m_queryResultsCount;
}