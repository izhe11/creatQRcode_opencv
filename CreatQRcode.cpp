#include "CreatQRcode.h"

CreatQRcode::CreatQRcode(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    statusLabel = new QLabel(" ", this);
    time = new QLabel("", this);
    ui.label_showPic->setAlignment(Qt::AlignCenter);  //居中显示
    ui.statusBar->addWidget(statusLabel);
    ui.statusBar->addWidget(time);
    statusLabel->setMinimumWidth(250);
    statusLabel->setMinimumHeight(25);
    time->setMinimumWidth(110);

    systemTime = new QTimer(this);         //刷新时间的计时器
    reflashTime();
    ui.lineEdit_input->setPlaceholderText("在此输入要写入二维码的内容...");

	connect(ui.creatQR, &QAction::triggered, this, &CreatQRcode::showQRPic);
    connect(ui.clear, &QAction::triggered, this, &CreatQRcode::clearContent);
    connect(ui.save, &QAction::triggered, this, &CreatQRcode::saveImage);
    connect(ui.open, &QAction::triggered, this, &CreatQRcode::openQRcode);
}

CreatQRcode::~CreatQRcode()
{
    delete systemTime;
    delete time;
    delete statusLabel;
}

//生成二维码，text为二维码内容
void CreatQRcode::creatQRcode(String text, Mat& qr)
{
    //配置二维码编码参数（使用默认即可）
    QRCodeEncoder::Params p;
    p.correction_level = QRCodeEncoder::CorrectionLevel::CORRECT_LEVEL_M;
    // 中文内容请固定使用 BYTE 模式，避免 AUTO 猜测模式导致兼容性问题
    p.mode = QRCodeEncoder::EncodeMode::MODE_BYTE;

    //创建编码器
    Ptr<QRCodeEncoder> encoder = QRCodeEncoder::create(p);
    if (encoder.empty())
    {
        statusLabel->setText("创建二维码失败（当前OpenCV可能不支持二维码编码）");
    }

    // 生成二维码矩阵
    encoder->encode(text, qr);  // qr 为单通道黑白图
    if (qr.empty())
    {
        statusLabel->setText("二维码生成失败！");
        return;
    }

    // 统一转灰度
    Mat qr_gray;
    if (qr.channels() == 1)
    {
        qr_gray = qr.clone();
    }
    else
    {
        cvtColor(qr, qr_gray, COLOR_BGR2GRAY);
    }
    if (qr_gray.depth() != CV_8U)
    {
        qr_gray.convertTo(qr_gray, CV_8U, 255.0);
    }

    // OTSU 自适应二值化，减少不同 OpenCV 输出差异
    Mat qr_bin;
    threshold(qr_gray, qr_bin, 0, 255, THRESH_BINARY | THRESH_OTSU);

    const int moduleCount = qr_bin.rows; // 行列均为模块数
    const int scale = 12;                // 每个模块像素
    const int quietModules = 8;          // 安静区模块数

    //将生成的二维码放大
    Mat qr_scaled;
    cv::resize(qr_bin, qr_scaled, Size(moduleCount * scale, moduleCount * scale), 0, 0, INTER_NEAREST);

    //添加安静区，即周围留白部分
    Mat qr_padded;
    copyMakeBorder(
        qr_scaled, qr_padded,
        quietModules * scale, quietModules * scale,
        quietModules * scale, quietModules * scale,
        BORDER_CONSTANT, Scalar(255)
    );

    qr = qr_padded;
}

//显示二维码
void CreatQRcode::showQRPic()
{
	Mat qr;
    QString qstr = ui.lineEdit_input->text();
    if (qstr.trimmed().isEmpty())
    {
        statusLabel->setText("输入内容为空，无法生成二维码！");
        ui.label_showPic->clear();
        return;
    }
    // 使用 UTF-8，避免本地编码导致二维码载荷异常
    std::string stdStr = qstr.toUtf8().toStdString();
    cv::String text = stdStr;
    
	//生成图片
	creatQRcode(text, qr);
	bool ok = imwrite("result.png", qr);
	if (!ok)
	{
        statusLabel->setText("保存二维码图片失败！");
		return;
	}

	//显示图片
	QPixmap pix("result.png");
	if (pix.isNull())
	{
        statusLabel->setText("二维码图片加载失败！");
		return;
	}

	QSize labelSize = ui.label_showPic->size();
	if (labelSize.width() < 20 || labelSize.height() < 20)
	{
		const int margin = 15;
		const int topOffset = 30;
		const int targetW = qMax(1, ui.groupBox_picture->width() - margin * 2);
		const int targetH = qMax(1, ui.groupBox_picture->height() - topOffset - margin);
		ui.label_showPic->setGeometry(margin, topOffset, targetW, targetH);
		labelSize = ui.label_showPic->size();
	}

	// 二维码显示必须保持“硬边缘”，避免平滑插值导致扫码失败
	ui.label_showPic->setPixmap(pix.scaled(labelSize, Qt::KeepAspectRatio, Qt::FastTransformation));
    statusLabel->setText("二维码生成完成！");
}

void CreatQRcode::clearContent()
{
    ui.lineEdit_input->clear();
    statusLabel->setText(" ");
    ui.label_showPic->clear();
    ui.textEdit_showText->clear();
}

//保存图片
void CreatQRcode::saveImage()
{
    //打开保存文件对话框，获取本地路径
    QString filepath = QFileDialog::getSaveFileName(this, "保存图片",
        QDir::homePath(), "Image(*.png *.jpg)");
    if (filepath.isEmpty())
    {
        return;
    }

    // Qt 6 中 QLabel::pixmap() 按值返回 QPixmap，不能用 ->，需用 .toImage()
    const QPixmap pix = ui.label_showPic->pixmap();
    if (pix.isNull())
    {
        QMessageBox::warning(this, "保存图片", "没有可保存的二维码，请先生成。");
        return;
    }
    QImage img = pix.toImage();
    if (!img.save(filepath))
    {
        QMessageBox::warning(this, "保存图片", "保存失败！");
    }
    statusLabel->setText("已保存");
}

//打开二维码图片
void CreatQRcode::openQRcode()
{
    QString filename = QFileDialog::getOpenFileName(this, "打开图片",
        QDir::homePath(), "Image(*.png *.jpg)");
    if (filename.isEmpty())
    {
        return;
    }
    QPixmap pix(filename);
    //缩放图片适应窗口
    QSize labelSize = ui.label_showPic->size();
    if (labelSize.width() > 350 || labelSize.height() > 330)
    {
        const int margin = 15;
        const int topOffset = 30;
        const int targetW = qMax(1, ui.groupBox_picture->width() - margin * 2);
        const int targetH = qMax(1, ui.groupBox_picture->height() - topOffset - margin);
        ui.label_showPic->setGeometry(margin, topOffset, targetW, targetH);
        labelSize = ui.label_showPic->size();
    }

    // 先把新图片设置到界面，再读取二维码内容
    QPixmap scaledPix = pix.scaled(labelSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    ui.label_showPic->setPixmap(scaledPix);

    String readData = readQRcode();
    ui.textEdit_showText->setText(QString::fromStdString(readData));
    // 设置字号：setFontWeight 只能设置字体粗细（weight），不是字号
    QFont f = ui.textEdit_showText->font();
    f.setPointSize(15); // 这里的单位是 point（不是像素）
    ui.textEdit_showText->setFont(f);

    if (readData.empty())
        statusLabel->setText("已打开图片，但未识别到二维码内容！");
    else
        statusLabel->setText("二维码识别成功！");
}

//读取二维码数据
String CreatQRcode::readQRcode()
{
    // 从界面上的 QLabel 取出当前显示的二维码图片
    const QPixmap currentPix = ui.label_showPic->pixmap();
    if (currentPix.isNull())
    {
        statusLabel->setText("请先生成或打开二维码图片！");
        return "";
    }

    QImage qImg = currentPix.toImage();
    if (qImg.isNull())
    {
        statusLabel->setText("二维码图片无效，无法识别！");
        return "";
    }

    // 转灰度后更利于二维码解码器识别
    QImage toGray = qImg.convertToFormat(QImage::Format_Grayscale8);
    Mat qrMatTemp(toGray.height(), toGray.width(), CV_8UC1,
                       const_cast<uchar*>(toGray.bits()),
                       toGray.bytesPerLine());
    Mat qrMatCopy = qrMatTemp.clone(); // clone() 确保 Mat 自己持有数据

    // 解码二维码内容
    QRCodeDetector detector;
    std::string decoded = detector.detectAndDecode(qrMatCopy);

    if (decoded.empty())
    {
        // 有些二维码在灰度/缩放后会反色或需要二值化；做几种简单重试,确保识别成功
        Mat qrBin;
        threshold(qrMatCopy, qrBin, 0, 255, THRESH_BINARY | THRESH_OTSU);
        decoded = detector.detectAndDecode(qrBin);

        if (decoded.empty())
        {
            Mat qrInv;
            bitwise_not(qrBin, qrInv);
            decoded = detector.detectAndDecode(qrInv);
        }
    }

    if (decoded.empty())
    {
        statusLabel->setText("未能识别二维码内容，请尝试更清晰的图片！");
        return "";
    }

    return String(decoded);
}

//获取系统时间
QString CreatQRcode::getSystemTime()
{
    QString myTime;         //存储系统时间用于返回值
    QDateTime currentTime = QDateTime::currentDateTime();

    //日期
    QDate date = currentTime.date();
    int year = date.year();
    int month = date.month();
    int day = date.day();

    //时间
    QTime time = currentTime.time();
    int hour = time.hour();
    int minite = time.minute();
    //int second = time.second();

    return myTime = QString("%1-%2-%3 %4:%5")
        .arg(year, 2, 10, QChar('0'))
        .arg(month, 2, 10, QChar('0'))
        .arg(day, 2, 10, QChar('0'))
        .arg(hour, 2, 10, QChar('0'))
        .arg(minite, 2, 10, QChar('0'));
        //.arg(second, 2, 10, QChar('0'));
}

void CreatQRcode::showTime()
{
    time->setText(getSystemTime());
}

void CreatQRcode::reflashTime()
{
    showTime();
    connect(systemTime, SIGNAL(timeout()), this, SLOT(showTime()));
    systemTime->start(1000);
}