#!/usr/bin/env php
<?php
#XTD:xspisi03
require_once 'ResourceManager.php';

function initVar(&$var, $val)
{
	if (!isset($var)) {
		$var = $val;
	}
}

abstract class Value
{
	const BIT = 1;
	const INT = 2;
	const FLOAT = 3;
	const NTEXT = 4;
	const NVARCHAR = 5;

	static $names = array(
		'UNDEFINED',
		'BIT',
		'INT',
		'FLOAT',
		'NVARCHAR',
		'NTEXT'
	);

	public static function toType($value)
	{
		if (filter_var($value, FILTER_VALIDATE_BOOLEAN, FILTER_NULL_ON_FAILURE) !== NULL) {
			return Value::BIT;
		} elseif (filter_var($value, FILTER_VALIDATE_INT) !== FALSE) {
			return Value::INT;
		} elseif (filter_var($value, FILTER_VALIDATE_FLOAT) !== FALSE) {
			return Value::FLOAT;
		}
		return Value::NTEXT;
	}

	public static function toString(&$value)
	{
		return self::$names[$value];
	}
}

class Main
{	
	private $resourceManager;

	private $ddlHeader;
	private $relationsExport = FALSE;
	private $ignoreAttributes = FALSE;
	private $mergeSameElements = FALSE;
	private $maxRelationCount = PHP_INT_MAX;
	private $helpMessage = <<<EOD
------------------------------------------------------------------------------
./src/xtd.py
===
--help                 - Show help
--input=filename.ext   - Input file with xml
--output=filename.ext  - Output file with xml
--header='text'        - Header included on the top of the output
--etc=num              - Maximal number of columns from the same element type
-a                     - Columns from attributes is not created
-b                     - More same elements seem like one
-g                     - XML with relations are on the output
------------------------------------------------------------------------------

EOD;

	function __construct()
	{
		$this->resourceManager = new ResourceManager();
	}

	public function parseOptions($argv)
	{
		$setParams = array();
		array_shift($argv);
		foreach ($argv as $option) {
			$param = explode("=", $option, 2);

			// Check duplicates
			if (isset($setParams[$param[0]])) {
				throw new Exception("Duplicate parameter: " . $param[0], 1);
			} else {
				$setParams[$param[0]] = TRUE;
			}

			if (strncmp($option, "--", 2) == 0) {
				if (strcmp($option, "--help") == 0) {
					echo $this->helpMessage;
					exit(0);
				} else {
					if (!isset($param[1])) {
						throw new Exception("Missing parameter value: " . $param[0], 1);
					}
					switch ($param[0]) {
						case '--input':
							$this->resourceManager->setInStreampath($param[1]);
							break;
						case '--output':
							$this->resourceManager->setOutStreampath($param[1]);
							break;
						case '--header':
							$this->ddlHeader = $param[1];
							break;
						case '--etc':
							$this->maxRelationCount = (int)$param[1];
							if ($this->maxRelationCount < 0) {
								throw new Exception("Wrong --etc parameter value.", 1);
							}
							break;

						default:
							throw new Exception("Undefined parameter: " . $option, 1);
							break;
					}
				}
			} else {
				switch ($param[0]) {
					case '-a':
						$this->ignoreAttributes = TRUE;
						break;
					case '-b':
						$this->mergeSameElements = TRUE;
						break;
					case '-g':
						$this->relationsExport = TRUE;
						break;
					default:
						throw new Exception("Undefined parameter: " . $option, 1);
						break;
				}
			}
		}

		if (($this->mergeSameElements) && ($this->maxRelationCount < PHP_INT_MAX)) {
			throw new Exception("Forbidden parameter combination -b and --etc.", 1);
		}
	}

	public function addColumn(&$tables, $table, $column, $type)
	{
		if (!isset($tables[$table][$column])) {
			$tables[$table][$column] = $type;
		} else {
			throw new Exception("Reserved column name conflict.", 90);
		}
	}

	public function parseXml(&$iterator, &$tables, &$counts, &$relations)
	{
		$refCount = array();
		$oldDepth = 0;
		foreach ($iterator as $element) {
			$tableName = $element->getName();
			initVar($tables[$tableName], array());

			// Parse new columns
			$newColumns = array();
			if (!$this->ignoreAttributes) {
				foreach ($element->attributes() as $columnName => $value) {
					$newColumns[strtolower($columnName)] = Value::toType($value);
				}
			}

			// Add text element as a 'value' column
			$value = (string) $element;
			if (($element->count() == 0) && (!ctype_space($value) && ($value != ''))) {
				$type = Value::toType($value);
				$type = ($type == Value::NTEXT) ? Value::NVARCHAR : $type;
				$newColumns['value'] = isset($newColumns['value']) ? max($newColumns['value'], $type) : $type;
			}

			// Merge new columns with existing columns
			foreach ($newColumns as $columnName => $columnType) {
				$tables[$tableName][$columnName] = isset($tables[$tableName][$columnName]) ? max($tables[$tableName][$columnName], $columnType) : $columnType;
			}

			$depth = $iterator->getDepth();

			// Clean reference counts
			if ($depth < $oldDepth) {
				for ($i = $oldDepth; $i > $depth; $i--) {
					unset($refCount[$i]);
				}
			}

			// We don't care about root->children relations :(
			if ($depth >= 1) {
				// Increase count
				if (!$this->mergeSameElements) {
					$refCount[$depth][$tableName] = isset($refCount[$depth][$tableName]) ? $refCount[$depth][$tableName] + 1 : 1;
				} else {
					$refCount[$depth][$tableName] = 1;
				}

				$parent = $iterator->getSubIterator($depth - 1)->key();

				// Set the count parent:children
				$counts[$parent][$tableName] = (isset($counts[$parent][$tableName])) ?
					max($counts[$parent][$tableName], $refCount[$depth][$tableName]) :
					$refCount[$depth][$tableName];

				// Init count children:parent if doesn't exist already
				initVar($counts[$tableName][$parent], 0);
			}

			$oldDepth = $depth;
		}

		// Add columns and relations
		foreach ($tables as $tableName => $columns) {
			// Add reflexive closure
			// BUT WHY?!?!
			initVar($relations[$tableName][$tableName], 1);

			if (!isset($counts[$tableName]))
				continue;

			foreach ($counts[$tableName] as $child => $count) {
				// check the limits
				if ($count <= $this->maxRelationCount) {
					if ($count > 0) {
						$relations[$tableName][$child] = 1;
						initVar($relations[$child][$tableName], 0);
						if ($count == 1) {
							$columnName = "${child}_id";
							$this->addColumn($tables, $tableName, $columnName, Value::INT);
						} else {
							for ($i = 1; $i <= $count; $i++) {
								$columnName = "${child}${i}_id";
								$this->addColumn($tables, $tableName, $columnName, Value::INT);
							}
						}
					}
				} else {
					$relations[$child][$tableName] = 1;
					initVar($relations[$tableName][$child], 0);
					$columnName = "${tableName}_id";
					$this->addColumn($tables, $child, $columnName, Value::INT);
				}
			}
		}
	}

	public function printDdl(&$tables)
	{
		$ddl = "";

		foreach ($tables as $tableName => $columns) {
			$ddl .= "CREATE TABLE $tableName(\n   prk_${tableName}_id INT PRIMARY KEY";

			// Add columns
			foreach ($columns as $columnName => $columnType) {
				$ddl .= ",\n   $columnName " . Value::toString($columnType);
			}				

			$ddl .= "\n);\n\n";
		}
		return $ddl;
	}

	public function printRelations(&$tables, &$relations)
	{
		$content = "<tables>\n";
		foreach ($tables as $tableName => $columns) {
			$content .= "    <table name=\"${tableName}\">\n";
			foreach ($relations[$tableName] as $child => $count) {
				
				if ((!empty($relations[$tableName][$child])) && (!empty($relations[$child][$tableName]))) {
					if ($tableName == $child) {
						// Not cheating.. not at all
						$relType = "1:1";
					} else {
						$relType = "N:M";
					}
				} elseif ((!empty($relations[$child][$tableName])) && (empty($relations[$tableName][$child]))) {
					$relType = "1:N";
				} elseif ((empty($relations[$child][$tableName])) && (!empty($relations[$tableName][$child]))) {
					$relType = "N:1";
				} else {
					$relType = "1:1";
				}

				$content .= "        <relation to=\"${child}\" relation_type=\"${relType}\" />\n";
			}
			$content .= "    </table>\n";
		}
		$content .= "</tables>\n";

		return $content;
	}

	public function run()
	{
		$fileContent = $this->resourceManager->readAll();

		$tables = array();
		$counts = array();
		$relations = array();
		$recXmlIt = new RecursiveIteratorIterator(new SimpleXMLIterator($fileContent), RecursiveIteratorIterator::SELF_FIRST);
		$this->parseXml($recXmlIt, $tables, $counts, $relations);

		$content = "";

		if (!$this->relationsExport) {
			// Print the header if set
			if ($this->ddlHeader) {
				$content .= "--$this->ddlHeader\n\n";
			}
			
			$content .= $this->printDdl($tables);
		} else {
			// Lets do the transitive closure
			do {
				$noChange = TRUE;
				foreach ($tables as $start => $columns) {
					foreach ($relations[$start] as $middle => &$middleVal) {
						foreach ($relations[$middle] as $end => &$endVal) {

							if (!isset($relations[$start][$end])) {// && ($start != $end)) {
								if (!empty($middleVal) && !empty($endVal)) {
									$relations[$start][$end] = 1;

									if (!empty($relations[$end][$middle]) || !empty($relations[$middle][$start])) {
										initVar($relations[$end][$start], 1);
									} else {
										initVar($relations[$end][$start], 0);
									}

									$noChange = FALSE;
								} elseif (!empty($middleVal)) {
									if (!empty($relations[$end][$middle])) {
										$relations[$start][$end] = 1;
										initVar($relations[$end][$start], 1);

										$noChange = FALSE;
									}
								} elseif (!empty($endVal)) {
									if (!empty($relations[$middle][$start])) {
										$relations[$start][$end] = 1;
										initVar($relations[$end][$start], 1);

										$noChange = FALSE;
									}
								}
							}

						}
					}
				}
			} while (!$noChange);

			// Print
			$content .= $this->printRelations($tables, $relations);
		}

		$this->resourceManager->writeAll($content);
	}
}

if ((php_sapi_name() == 'cli') && empty($_SERVER['REMOTE_ADDR'])) {
	$ret = 0;
	$main = new Main();
	try {
		$main->parseOptions($argv);
		$main->run();
	} catch (Exception $e) {
		fprintf(STDERR, "E: (" . $e->getCode() . ") " . $e->getMessage() . "\n");
		$ret = $e->getCode();
	}

	exit($ret);
}

?>