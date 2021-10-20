
class Machine
{

	private:
	class Adapter* _platformAdapter;

	public:
		void setPlatformAdapter(class Adapter *platformAdapter);
		void playSound();
};