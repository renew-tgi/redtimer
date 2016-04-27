#include "IssueSelector.h"
#include "logging.h"

using namespace qtredmine;
using namespace redtimer;
using namespace std;

IssueSelector::IssueSelector( SimpleRedmineClient* redmine, QQuickView* parent )
    : Window( "qrc:/IssueSelector.qml", parent ),
      redmine_( redmine )
{
    ENTER();

    // Issue selector window initialisation
    setResizeMode( QQuickView::SizeRootObjectToView );
    setModality( Qt::ApplicationModal );
    setFlags( Qt::Dialog );
    setTitle( "Issue Selector" );

    // Set the models
    issuesProxyModel_.setSourceModel( &issuesModel_ );
    issuesProxyModel_.setSortRole( IssueModel::IssueRoles::IdRole );
    issuesProxyModel_.setDynamicSortFilter( true );
    issuesProxyModel_.setFilterCaseSensitivity( Qt::CaseInsensitive );
    issuesProxyModel_.setFilterRole( IssueModel::IssueRoles::SubjectRole );
    ctx_->setContextProperty( "issuesModel", &issuesProxyModel_ );

    QSortFilterProxyModel* projectProxyModel = new QSortFilterProxyModel();
    projectProxyModel->setSourceModel( &projectModel_ );
    projectProxyModel->setSortRole( SimpleModel::SimpleRoles::IdRole );
    projectProxyModel->setDynamicSortFilter( true );
    ctx_->setContextProperty( "projectModel", projectProxyModel );

    // Connect the project selected signal to the projectSelected slot
    connect( qml("project"), SIGNAL(activated(int)), this, SLOT(projectSelected(int)) );

    // Connect the issue selected signal to the issueSelected slot
    connect( qml("issues"), SIGNAL(activated(int)), this, SLOT(issueSelected(int)) );

    // Connect the search accepted signal to the filterIssues slot
    connect( qml("search"), SIGNAL(textChanged()), this, SLOT(filterIssues()) );

    RETURN();
}

void
IssueSelector::display()
{
    ENTER();

    if( !isVisible() )
    {
        DEBUG() << "Displaying issue selector window";
        show();
        qml("search")->setProperty( "text", "" );
        loadProjects();
    }

    RETURN();
}

void
IssueSelector::filterIssues()
{
    ENTER();

    QString filter = qml("search")->property("text").toString();
    issuesProxyModel_.setFilterFixedString( filter );

    RETURN();
}

void
IssueSelector::issueSelected( int index )
{
    ENTER();

    int issueId = issuesModel_.at(index).id;
    DEBUG()(index)(issueId);

    selected( issueId );

    close();

    RETURN();
}

void
IssueSelector::projectSelected( int index )
{
    ENTER();

    projectId_ = projectModel_.at(index).id();
    DEBUG()(index)(projectId_);

    loadIssues();

    RETURN();
}

void
IssueSelector::loadIssues()
{
    ENTER()(projectId_);

    redmine_->retrieveIssues( [&]( Issues issues )
    {
        ENTER();

        issuesModel_.clear();

        for( const auto& issue : issues )
            issuesModel_.push_back( issue );

        DEBUG()(issuesModel_);
    },
    RedmineOptions( QString("project_id=%1").arg(projectId_), true ) );

    RETURN();
}

void
IssueSelector::loadProjects()
{
    ENTER();

    // Clear and set first item at once and not wait for callback
    projectModel_.clear();
    projectModel_.push_back( SimpleItem(NULL_ID, "Choose project") );

    redmine_->retrieveProjects( [=]( Projects projects )
    {
        ENTER();

        int currentIndex = 0;

        // Reset in case this has changed since calling loadProjects()
        projectModel_.clear();
        projectModel_.push_back( SimpleItem(NULL_ID, "Choose project") );

        for( const auto& project : projects )
        {
            if( project.id == projectId_ )
                currentIndex = projectModel_.rowCount();

            projectModel_.push_back( SimpleItem(project) );
        }

        DEBUG()(projectModel_)(currentIndex);

        qml("project")->setProperty( "currentIndex", -1 );
        qml("project")->setProperty( "currentIndex", currentIndex );

        RETURN();
    },
    QString("limit=100") );
}

int
IssueSelector::getProjectId() const
{
    ENTER();
    RETURN( projectId_ );
}

void
IssueSelector::setProjectId( int id )
{
    ENTER();
    projectId_ = id;
    loadIssues();
    RETURN();
}