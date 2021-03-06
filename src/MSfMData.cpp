#include "MSfMData.hpp"

#include <QThreadPool>
#include <QFileInfo>
#include <QDebug>

#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

namespace qtAliceVision {

/**
 * @brief QRunnable object dedicated to load sfmData using AliceVision.
 */
class SfmDataIORunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit SfmDataIORunnable(const QUrl& sfmDataPath, aliceVision::sfmData::SfMData* sfmData)
    : _sfmDataPath(sfmDataPath)
    , _sfmData(sfmData)
    {}

    /// Load SfM based on input parameters
    Q_SLOT void run() override;

    /**
     * @brief  Emitted when sfmData have been loaded and sfmData objects created.
     */
    Q_SIGNAL void resultReady();

private:
    const QUrl _sfmDataPath;
    aliceVision::sfmData::SfMData* _sfmData;
};

void SfmDataIORunnable::run()
{
    using namespace aliceVision;

    // std::unique_ptr<sfmData::SfMData> sfmData(new sfmData::SfMData());
    try
    {
        if(!sfmDataIO::Load(*_sfmData, _sfmDataPath.toLocalFile().toStdString(), sfmDataIO::ESfMData::ALL))
        {
            qDebug() << "[QtAliceVision] Failed to load sfmData: " << _sfmDataPath << ".";
        }
    }
    catch(std::exception& e)
    {
        qDebug() << "[QtAliceVision] Failed to load sfmData: " << _sfmDataPath << "."
                 << "\n" << e.what();
    }

    Q_EMIT resultReady(); //sfmData.release());
}

MSfMData::MSfMData()
    : _sfmData(new aliceVision::sfmData::SfMData())
    , _loadingSfmData(new aliceVision::sfmData::SfMData())
{
}

MSfMData::~MSfMData()
{
    setStatus(None);
    clear();
}

void MSfMData::clear()
{
    _sfmData->clear();
    Q_EMIT sfmDataChanged();
}

void MSfMData::load()
{
    if (status() == Loading)
    {
        qWarning("Failed to load, a load event is already running.");
        return;
    }
    if(_sfmDataPath.isEmpty())
    {
        setStatus(None);
        clear();
        return;
    }
    if(!QFileInfo::exists(_sfmDataPath.toLocalFile()))
    {
        setStatus(Error);
        clear();
        return;
    }

    setStatus(Loading);

    _loadingSfmData->clear();
    // load features from file in a seperate thread
    SfmDataIORunnable* ioRunnable = new SfmDataIORunnable(_sfmDataPath, _loadingSfmData.get());
    connect(ioRunnable, &SfmDataIORunnable::resultReady, this, &MSfMData::onSfmDataReady);
    QThreadPool::globalInstance()->start(ioRunnable);
}

void MSfMData::onSfmDataReady()
{
    if(!_loadingSfmData)
        return;
    setStatus(Loading);
    _sfmData.swap(_loadingSfmData);
    _loadingSfmData->clear();
    setStatus(Ready);
}

}

#include "MSfMData.moc"
