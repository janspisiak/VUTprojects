<?php
class ResourceManager
{
	private $inStreampath;
	private $outStreampath;
	private $errHandle;

	static private $instance;

	function __construct()
	{
		$this->inStreampath = "php://stdin";
		$this->outStreampath = "php://stdout";
		$this->errHandle = STDERR;

		self::$instance = $this;
	}

	public function setInStreampath($inStreampath)
	{
		$this->inStreampath = $inStreampath;
		return $inStreampath;
	}

	public function setOutStreampath($outStreampath)
	{
		$this->outStreampath = $outStreampath;
		return $outStreampath;
	}

	public function setErrHandle($errHandle)
	{
		$this->errHandle = $errHandle;
		return $errHandle;
	}
 

	public function readAll()
	{
		$data = file_get_contents($this->inStreampath);
		if ($data === FALSE) {
			throw new Exception("Couldn't read input.", 2);
		}
		return $data;
	}

	public function writeAll($string)
	{
		$data = file_put_contents($this->outStreampath, $string);
		if ($data === FALSE) {
			throw new Exception("Couldn't write to output.", 3);
		}
		return $data;
	}


	public static function getInstance()
	{
		return self::$instance;
	}
}
?>