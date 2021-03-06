#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <memory>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>

static const double  PI2 = M_PI * 2;

static const int dtmf_fq[8] = { 697, 770, 852, 941, 1209, 1336, 1477, 1633 };

double goertzel(int size, int16_t const *data, int sample_fq, int detect_fq)
{
	double omega = PI2 * detect_fq / sample_fq;
	double sine = sin(omega);
	double cosine = cos(omega);
	double coeff = cosine * 2;
	double q0 = 0;
	double q1 = 0;
	double q2 = 0;

	for (int i = 0; i < size; i++) {
		q0 = coeff * q1 - q2 + data[i];
		q2 = q1;
		q1 = q0;
	}

	double real = (q1 - q2 * cosine) / (size / 2.0);
	double imag = (q2 * sine) / (size / 2.0);

	return sqrt(real * real + imag * imag);
}

struct MainWindow::Private {
	bool playing = false;
	int volume = 5000;
	int sample_fq = 8000;
	int tone_fq_lo = 0;
	int tone_fq_hi = 0;
	double phase_lo = 0;
	double phase_hi = 0;
	std::shared_ptr<QAudioOutput> audio_output;
	QIODevice *device = nullptr;
	double dtmf_levels[8] = {};
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);

	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(1);
	format.setCodec("audio/pcm");
	format.setSampleRate(m->sample_fq);
	format.setSampleSize(16);
	format.setSampleType(QAudioFormat::SignedInt);
	m->audio_output = std::make_shared<QAudioOutput>(format);
	m->audio_output->setBufferSize(2000);
	connect(m->audio_output.get(), &QAudioOutput::notify, this, &MainWindow::outputAudio);
	m->device = m->audio_output->start();

	startTimer(10);
}

MainWindow::~MainWindow()
{
	delete m;
	delete ui;
}

void MainWindow::detectDTMF(int size, int16_t const *data)
{
	for (int i = 0; i < 8; i++) {
		m->dtmf_levels[i] = goertzel(size, data, m->sample_fq, dtmf_fq[i]);
	}
}

void MainWindow::outputAudio()
{
	if (!m->playing) {
		for (int i = 0; i < 8; i++) {
			m->dtmf_levels[i] = 0;
		}
		return;
	}

	std::vector<int16_t> buf;

	while (1) {
		int n = m->audio_output->bytesFree();
		n /= sizeof(int16_t);
		const int N = 96;
		if (n < N) return;

		buf.resize(n);

		double add_lo = PI2 * m->tone_fq_lo / m->sample_fq;
		double add_hi = PI2 * m->tone_fq_hi / m->sample_fq;
		for (int i = 0; i < n; i++) {
			int v = 0;
			if (add_lo != 0) v += sin(m->phase_lo) * m->volume;
			if (add_hi != 0) v += sin(m->phase_hi) * m->volume;
			buf[i] = v;
			m->phase_lo += add_lo;
			m->phase_hi += add_hi;
			while (m->phase_lo >= PI2) m->phase_lo -= PI2;
			while (m->phase_hi >= PI2) m->phase_hi -= PI2;
		}

		m->device->write((char const *)&buf[0], n * sizeof(int16_t));

		detectDTMF(buf.size(), &buf[0]);
	}
}

void MainWindow::timerEvent(QTimerEvent *)
{
	outputAudio();

	QProgressBar *pb[8] = {
		ui->progressBar_1,
		ui->progressBar_2,
		ui->progressBar_3,
		ui->progressBar_4,
		ui->progressBar_5,
		ui->progressBar_6,
		ui->progressBar_7,
		ui->progressBar_8,
	};

	for (int i = 0; i < 8; i++) {
		pb[i]->setRange(0, 10000);
		pb[i]->setValue(m->dtmf_levels[i]);
	}
}

void MainWindow::setTone(char c)
{
	switch (c) {
	case '1':
	case '2':
	case '3':
	case 'A':
		m->tone_fq_lo = 697;
		break;
	case '4':
	case '5':
	case '6':
	case 'B':
		m->tone_fq_lo = 770;
		break;
	case '7':
	case '8':
	case '9':
	case 'C':
		m->tone_fq_lo = 852;
		break;
	case '*':
	case '0':
	case '#':
	case 'D':
		m->tone_fq_lo = 941;
		break;
	}
	switch (c) {
	case '1':
	case '4':
	case '7':
	case '*':
		m->tone_fq_hi = 1209;
		break;
	case '2':
	case '5':
	case '8':
	case '0':
		m->tone_fq_hi = 1336;
		break;
	case '3':
	case '6':
	case '9':
	case '#':
		m->tone_fq_hi = 1477;
		break;
	case 'A':
	case 'B':
	case 'C':
	case 'D':
		m->tone_fq_hi = 1633;
		break;
	}
}

void MainWindow::on_pushButton_clicked()
{
	m->playing = !m->playing;
}

void MainWindow::on_toolButton_pressed()
{
	m->playing = true;
}

void MainWindow::on_toolButton_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_1_pressed()
{
	setTone('1');
	m->playing = true;
}

void MainWindow::on_toolButton_1_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_2_pressed()
{
	setTone('2');
	m->playing = true;
}

void MainWindow::on_toolButton_2_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_3_pressed()
{
	setTone('3');
	m->playing = true;
}

void MainWindow::on_toolButton_3_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_4_pressed()
{
	setTone('4');
	m->playing = true;
}

void MainWindow::on_toolButton_4_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_5_pressed()
{
	setTone('5');
	m->playing = true;
}

void MainWindow::on_toolButton_5_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_6_pressed()
{
	setTone('6');
	m->playing = true;
}

void MainWindow::on_toolButton_6_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_7_pressed()
{
	setTone('7');
	m->playing = true;
}

void MainWindow::on_toolButton_7_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_8_pressed()
{
	setTone('8');
	m->playing = true;
}

void MainWindow::on_toolButton_8_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_9_pressed()
{
	setTone('9');
	m->playing = true;
}

void MainWindow::on_toolButton_9_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_10_pressed()
{
	setTone('*');
	m->playing = true;
}

void MainWindow::on_toolButton_10_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_11_pressed()
{
	setTone('0');
	m->playing = true;
}

void MainWindow::on_toolButton_11_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_12_pressed()
{
	setTone('#');
	m->playing = true;
}

void MainWindow::on_toolButton_12_released()
{
	m->playing = false;
}


void MainWindow::on_toolButton_a_pressed()
{
	setTone('A');
	m->playing = true;
}

void MainWindow::on_toolButton_a_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_b_pressed()
{
	setTone('B');
	m->playing = true;
}

void MainWindow::on_toolButton_b_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_c_pressed()
{
	setTone('C');
	m->playing = true;
}

void MainWindow::on_toolButton_c_released()
{
	m->playing = false;
}

void MainWindow::on_toolButton_d_pressed()
{
	setTone('D');
	m->playing = true;
}

void MainWindow::on_toolButton_d_released()
{
	m->playing = false;
}

