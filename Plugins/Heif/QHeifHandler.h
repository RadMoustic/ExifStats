#pragma once

// https://github.com/jakar/qt-heif-image-plugin

#include <libheif/heif.h>

#include <QtCore/QIODevice>
#include <QtGui/QImageIOHandler>

#include <memory>
#include <vector>

class QHeifHandler : public QImageIOHandler
{
public:
	enum class Format
	{
		None,
		Heif,
		HeifSequence,
		Heic,
		HeicSequence,
	};

	explicit QHeifHandler();
	virtual ~QHeifHandler();

	QHeifHandler(const QHeifHandler& handler) = delete;
	QHeifHandler& operator=(const QHeifHandler& handler) = delete;

	bool canRead() const override;
	bool read(QImage* image) override;

	bool write(const QImage& image) override;

	int currentImageNumber() const override;
	int imageCount() const override;
	bool jumpToImage(int index) override;
	bool jumpToNextImage() override;

	QVariant option(ImageOption opt) const override;
	void setOption(ImageOption opt, const QVariant& value) override;
	bool supportsOption(ImageOption opt) const override;

	static Format canReadFrom(QIODevice& device);

private:
	struct ReadState
	{
		ReadState(QByteArray&& data,
				  std::shared_ptr<heif_context>&& ctx,
				  std::vector<heif_item_id>&& ids,
				  int index);

		const QByteArray fileData;
		const std::shared_ptr<heif_context> context;
		const std::vector<heif_item_id> idList;
		int currentIndex{};
	};

	/**
	 * Updates device and associated state upon device change.
	 */
	void updateDevice();

	/**
	 * Reads data from device. Creates read state.
	 */
	void loadContext();

	QIODevice* mDevice = nullptr;
	std::unique_ptr<ReadState> mReadState;  // non-null iff context is loaded
	int mQuality;
};
