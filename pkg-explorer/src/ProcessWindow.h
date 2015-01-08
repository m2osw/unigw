#pragma once

class ProcessWindow : public QObject
{
	Q_OBJECT

	public:
		ProcessWindow() {}

	public slots:
		void ShowProcessDialog( const bool show_it, const bool enable_cancel )
		{
			if( show_it )
			{
				f_procDlg.show();
				f_procDlg.EnableCancelButton( enable_cancel );
			}
			else
			{
				f_procDlg.hide();
			}
		}

	private:
		ProcessDialog	f_procDlg;
};
